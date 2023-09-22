import os
import sys
import json
import glob
import zstandard
import tqdm
from pathlib import Path

text_base_dir = "/mnt/disk01/work/japanese-dataset-cleaned-experiment/02_clean_step"
text_corpus_list = {
  "cc100ja": ["cc100ja", "text", "*.jsonl.zstd"],
  "mc4": ["mc4", "text", "*.json.zstd"],
  "oscar": ["OSCAR-2301", "content", "*.jsonl.zstd"]
}

lm_score_dir = "/mnt/disk01/work/japanese-dataset-cleaned-experiment/04_lm_scoring/"
dedup_dir = "/mnt/disk01/work/japanese-dataset-cleaned-experiment/06_dedup/"

out_dir = "/mnt/disk01/work/japanese-dataset-cleaned-experiment/beauty/"
dataset_basename = "japanese-corpus-{:05d}.jsonl.zstd"

ndocs_per_file = 25600
nfiles_in_chunk = 128

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

def save_to_chunk(out_dir, basefilename, jsons):


    nfiles = 0
    file_i = 0
    chunk_i = 0

    os.makedirs(os.path.join(out_dir, "chunk_0"), exist_ok=True)

    for i in range(0, len(jsons), ndocs_per_file):
        js = jsons[i:i+ndocs_per_file]

        zfilename = os.path.join(out_dir, os.path.join("chunk_{}".format(chunk_i), basefilename.format(file_i)))

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

        nfiles += 1
        file_i += 1

        if nfiles >= nfiles_in_chunk:
            print("new chunk!")
            nfiles = 0
            file_i = 0
            chunk_i += 1

            os.makedirs(os.path.join(out_dir, "chunk_{}".format(chunk_i)), exist_ok=True)




corpus_list = text_corpus_list

if len(sys.argv) > 1:
    corpus_list = [sys.argv[1]] # for debugging

jsons = []
for corpus in corpus_list:
    corpus_dirname = text_corpus_list[corpus][0]
    text_key = text_corpus_list[corpus][1]
    glob_pattern = text_corpus_list[corpus][2]

    files = glob.glob(os.path.join(text_base_dir, os.path.join(corpus_dirname, glob_pattern)))

    # hack
    n = 4

    for f in tqdm.tqdm(files[:n]):
        basefilename = os.path.basename(f)

        score_file = Path(os.path.join(os.path.join(lm_score_dir, corpus_dirname), basefilename))
        if not score_file.exists():
            print("lm score file not found: ", score_file)

        dedup_file = Path(os.path.join(dedup_dir, basefilename))
        if not dedup_file.exists():
            print("dedup file not found: ", dedup_file)

        jsons += merge_jsonl(f, score_file, dedup_file, text_key)


    jsons = sorted(jsons, key=lambda j: j["lm_score"])

    save_to_chunk(out_dir, dataset_basename, jsons)
