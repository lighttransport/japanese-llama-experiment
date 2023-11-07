import os
import sys
import json
import zstandard
import tqdm
import pandas as pd
import numpy as np
from pathlib import Path

from datasets import load_dataset, DatasetDict

out_dir = "/mnt/data/work/Japanese-CharShu-59B/"
dataset_basenames = {
    'train': "japanese-corpus-{:05d}.jsonl.zstd",
    'validate': "japanese-corpus-validate-{:05d}.jsonl.zstd",
    'test': "japanese-corpus-holdout-{:05d}.jsonl.zstd"}

ndocs_per_file = 25600
lm_score_num_bins = 32
zstd_comp_level = 5

# Edit path to your local dir.
dataset = load_dataset("lighttransport/Japanese-CharShu-59B", num_proc=8)

# We choose 1% each(2% in total), based on SlimPajama
# https://github.com/Cerebras/modelzoo/blob/97bdaf4460ace1681ad437b07ba33f0e179f5ca4/modelzoo/transformers/data_processing/slimpajama/preprocessing/shuffle_holdout.py#L110

test_and_validate_percentage = 0.02

# https://discuss.huggingface.co/t/how-to-split-main-dataset-into-train-dev-test-as-datasetdict/1090/7

# 2% for validate & test
train_testvalid = dataset['train'].train_test_split(test_size=test_and_validate_percentage)

test_valid = train_testvalid['test'].train_test_split(test_size=0.5)

train_test_valid_dataset = DatasetDict(
    { 'train': train_testvalid['train'],
      'validate': test_valid['test'],
      'test': test_valid['train']
    })

# TODO: ensure 'test' dataset is not included in 'train' split

#print(train_test_valid_dataset)

def save_remainder_docs(bin_dict, out_dir, basefilename):

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

def compute_lm_score_bins(datasets, nbins, step=1):

    lm_min = float("inf");
    lm_max = 0.0

    lm_scores = []

    for i in tqdm.tqdm(range(0, len(datasets), step)):
            score = datasets[i]["lm_score"]
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

    return d

def save_dataset(label, datasets, bins):
    bin_dict = {}
    for i in range(len(bins)):
        bin_dict[i] = { 'jsons': [], 'file_count': 0 }

    save_to_chunk(bin_dict, bins, os.path.join(out_dir, label), dataset_basenames[label], datasets)
    save_remainder_docs(bin_dict, os.path.join(out_dir, label), dataset_basenames[label])
    

#step = 50 # test
step = 1 # production
lm_score_bins = compute_lm_score_bins(train_test_valid_dataset['train'], lm_score_num_bins, step)

save_dataset('train', train_test_valid_dataset['train'], lm_score_bins['bins'])
save_dataset('validate', train_test_valid_dataset['validate'], lm_score_bins['bins'])
save_dataset('test', train_test_valid_dataset['test'], lm_score_bins['bins'])



