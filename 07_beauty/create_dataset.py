import os
import sys
import json
import glob
import zstandard
import tqdm
import numpy as np
import pandas as pd
import bisect
from pathlib import Path

text_base_dir = "/mnt/disk01/work/japanese-dataset-cleaned-experiment/02_clean_step"
text_corpus_list = {
  "cc100ja": ["cc100ja", "text", "*.jsonl.zstd"],
  "mc4": ["mc4", "text", "*.json.zstd"],
  "oscar": ["OSCAR-2301", "content", "*.jsonl.zstd"]
}

lm_score_dir = "/mnt/disk01/work/japanese-dataset-cleaned-experiment/04_lm_scoring/"
dedup_dir = "/mnt/disk01/work/japanese-dataset-cleaned-experiment/06_dedup/"
lm_score_bins_file = "lm_score_bins.json"
lm_score_num_bins = 32

out_dir = "/mnt/disk01/work/japanese-dataset-cleaned-experiment/beauty/"
dataset_basename = "japanese-corpus-{:05d}.jsonl.zstd"

ndocs_per_file = 25600

zstd_comp_level = 5

def load_jsonl_zstd(filepath):

    with open(filepath, 'rb') as f:
        indata = f.read()

    dctx = zstandard.ZstdDecompressor()
    dobj = dctx.decompressobj()
    jsonldata = dobj.decompress(indata)

    lines = jsonldata.splitlines()
    del indata

    jsons = []

    for line in lines:
        if len(line) < 1:
            continue

        j = json.loads(line)
    
        jsons.append(j)


    return jsons

def merge_jsonl(text_file, score_file, dedup_file, text_key):
    print("merge", text_file, score_file, dedup_file)

    text_jsonl = load_jsonl_zstd(text_file)
    score_jsonl = load_jsonl_zstd(score_file)
    dedup_jsonl = load_jsonl_zstd(dedup_file)
    
    if len(text_jsonl) != len(score_jsonl):
        print(text_jsonl[-1])
        print(score_jsonl[-1])
        raise RuntimeError("jsonl lines mismatch", text_file, len(text_jsonl), score_file, len(score_jsonl))

    if len(text_jsonl) != len(dedup_jsonl):
        print(text_jsonl[-1])
        print(dedup_jsonl[-1])
        raise RuntimeError("jsonl lines mismatch", text_file, len(text_jsonl), dedup_file, len(dedup_jsonl))

    jsons = []
    for i in range(len(text_jsonl)):
        if dedup_jsonl[i]["duplicate"]:
            continue

        j = {}
        j["text"] = text_jsonl[i][text_key]
        j["lm_score"] = score_jsonl[i]["lm_score"]

        jsons.append(j)

    score_sorted = sorted(jsons, key=lambda j: j["lm_score"])
    return score_sorted 

def save_remainder_docs(bin_dict, out_dir, basefilename, jsons):

    for bin_idx in bin_dict:
        js = bin_dict[bin_idx]['jsons']

        if len(js)  == 0:
            continue

        file_i = bin_dict[bin_idx]['file_count']

        os.makedirs(os.path.join(out_dir, "chunk_{}".format(bin_idx)), exist_ok=True)
        zfilename = os.path.join(out_dir, os.path.join("chunk_{}".format(bin_idx), basefilename.format(file_i)))


        zctx = zstandard.ZstdCompressor(level=zstd_comp_level)

        dst_lines = []
        for j in js:
            dst_lines.append(json.dumps(j, ensure_ascii=False))

        dst_buf = "\n".join(dst_lines)

        del dst_lines

        # TODO: Use stream
        zcompressed = zctx.compress(bytes(dst_buf, 'utf-8'))

        print("write to ", zfilename)
        of = open(zfilename, 'wb')
        of.write(zcompressed)
        of.close()

        del dst_buf
        del zcompressed
    

def save_to_chunk(bin_dict, bins, out_dir, basefilename, jsons):

    for i in range(len(jsons)):

        j = jsons[i]
        lm_score = j['lm_score']

        bin_idx = np.digitize(lm_score, bins)

        # just in case.
        bin_idx = max(0, bin_idx)
        bin_idx = min(len(bins)-1, bin_idx)

        bin_dict[bin_idx]['jsons'].append(j)

        ndocs = len(bin_dict[bin_idx]['jsons'])

        if ndocs >= ndocs_per_file:

            file_i = bin_dict[bin_idx]['file_count']

            os.makedirs(os.path.join(out_dir, "chunk_{}".format(bin_idx)), exist_ok=True)
            zfilename = os.path.join(out_dir, os.path.join("chunk_{}".format(bin_idx), basefilename.format(file_i)))

            js = bin_dict[bin_idx]['jsons']

            zctx = zstandard.ZstdCompressor(level=zstd_comp_level)

            dst_lines = []
            for j in js:
                dst_lines.append(json.dumps(j, ensure_ascii=False))

            dst_buf = "\n".join(dst_lines)

            del dst_lines

            # TODO: Use stream
            zcompressed = zctx.compress(bytes(dst_buf, 'utf-8'))

            print("write to ", zfilename)
            of = open(zfilename, 'wb')
            of.write(zcompressed)
            of.close()

            del dst_buf
            del zcompressed

            del bin_dict[bin_idx]['jsons']
            bin_dict[bin_idx]['jsons'] = []

            bin_dict[bin_idx]['file_count'] += 1


def compute_lm_score_bins(corpus_list, nbins):

    lm_min = float("inf");
    lm_max = 0.0

    lm_scores = []

    for corpus in corpus_list:
        corpus_dirname = text_corpus_list[corpus][0]
        text_key = text_corpus_list[corpus][1]
        glob_pattern = text_corpus_list[corpus][2]

        files = glob.glob(os.path.join(text_base_dir, os.path.join(corpus_dirname, glob_pattern)))

        for f in tqdm.tqdm(files):
            basefilename = os.path.basename(f)

            score_file = Path(os.path.join(os.path.join(lm_score_dir, corpus_dirname), basefilename))
            if not score_file.exists():
                print("lm score file not found: ", score_file)


            jsons = load_jsonl_zstd(score_file)
            for j in jsons:
                score = j["lm_score"]
                lm_min = min(score, lm_min)
                lm_max = max(score, lm_max)

                # NOTE: we can skip adding score to a list for each N(say 10)
                # This won't hurt the precision of finding bins for larger samples(1M or more).
                lm_scores.append(score)

    # Use pandas.qcut to discretize lm_score into equal-sized buckets
    # https://pandas.pydata.org/docs/reference/api/pandas.qcut.html

    df, bins = pd.qcut(lm_scores, nbins, duplicates='drop', retbins=True)
    print(df)

    bs = []
    for b in bins:
        bs.append(float(b))

    d = {}
    d["bins"] = bs
    d["lm_min"] = lm_min
    d["lm_max"] = lm_max

    with open(lm_score_bins_file, 'w') as f:
        f.write(json.dumps(d))


corpus_list = text_corpus_list

if len(sys.argv) > 1:
    corpus_list = [sys.argv[1]] # for debugging

if not Path(lm_score_bins_file).exists():
    compute_lm_score_bins(corpus_list, lm_score_num_bins)

with open(lm_score_bins_file, 'r') as f:
    j = json.loads(f.read())

    bins = j['bins']


# track jsonl document per bin
bin_dict = {}
for i in range(len(bins)):
    bin_dict[i] = { 'jsons': [], 'file_count': 0 }

jsons = []
for corpus in corpus_list:
    corpus_dirname = text_corpus_list[corpus][0]
    text_key = text_corpus_list[corpus][1]
    glob_pattern = text_corpus_list[corpus][2]

    files = glob.glob(os.path.join(text_base_dir, os.path.join(corpus_dirname, glob_pattern)))

    for f in tqdm.tqdm(files):
        basefilename = os.path.basename(f)

        score_file = Path(os.path.join(os.path.join(lm_score_dir, corpus_dirname), basefilename))
        if not score_file.exists():
            print("lm score file not found: ", score_file)

        dedup_file = Path(os.path.join(dedup_dir, basefilename))
        if not dedup_file.exists():
            print("dedup file not found: ", dedup_file)

        jsons = merge_jsonl(f, score_file, dedup_file, text_key)

        save_to_chunk(bin_dict, bins, out_dir, dataset_basename, jsons)

    save_remainder_docs(bin_dict, out_dir, dataset_basename, jsons)
