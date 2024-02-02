# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2023 - Present, Light Transport Entertainment Inc.
import sys
import gzip
import json
import unicodedata
import glob
import os
from pathlib import Path
import hashlib
import concurrent.futures

import zstandard
import text_normalizer
from tqdm import tqdm

zstd_comp_level = 4 # default = 3

# TODO: hash check

mc4_glob_pattern = "../data/00_dataset/c4/multilingual/c4-ja.tfrecord-{:05d}-of-01024.json.gz"
dst_mc4_path = Path("../data/01_normalized/mc4")

# Create directory if not exists.
os.makedirs(dst_mc4_path, exist_ok=True)

nprocesses = 6


def worker(filepath):
    print("Processing ", filepath)

    f = gzip.open(filepath, 'rb')
    line=f.readline()

    dst_lines = []

    while line:

        j = json.loads(line)

        # Simple version NFKC
        #j["text"] = unicodedata.normalize('NFKC', j["text"])

        # cc_net compatible version. apply NFKC normalization also. 
        j["text"] = text_normalizer.normalize(j["text"])

        dst_lines.append(json.dumps(j, ensure_ascii=False))

        line = f.readline()

    f.close()

    #print(lines[0])

    zctx = zstandard.ZstdCompressor(level=zstd_comp_level)
    zfilename = os.path.join(dst_mc4_path, os.path.splitext(os.path.basename(filepath))[0] + ".zstd")

    dst_buf = "\n".join(dst_lines)

    # TODO: Use stream

    zcompressed = zctx.compress(bytes(dst_buf, 'utf-8'))

    print("write to ", zfilename)
    of = open(zfilename, 'wb')
    of.write(zcompressed)
    of.close()

#
# - main
# 
offset = 0
n = 1024

if len(sys.argv) > 2:
    offset = int(sys.argv[1])
    n = int(sys.argv[2])

    assert offset >= 0
    assert offset < 1024
    assert n > 0

assert (offset + n) <= 1024

inputfiles = []
for i in range(n):
    idx = offset + i
    filepath = mc4_glob_pattern.format(idx)
    inputfiles.append(filepath)

with concurrent.futures.ProcessPoolExecutor(max_workers=nprocesses) as executor:
    fs = list(tqdm(executor.map(worker, inputfiles), total=len(inputfiles)))
