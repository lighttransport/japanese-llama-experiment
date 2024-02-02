# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2024 - Present, Light Transport Entertainment Inc.
import sys
import gzip
import json
import unicodedata
import glob
import os
import hashlib
import concurrent.futures
#from multiprocessing import Pool
from pathlib import Path

import zstandard
import text_normalizer
from tqdm import tqdm

zstd_comp_level = 4 # default = 3

nfiles = 12
wiki40b_glob_pattern = "../data/00_dataset/wiki40b-ja/wiki40b-ja.{:05d}.jsonl.zstd"
#checksumfile = ""
dst_wiki40b_path = Path("../data/01_normalized/wiki40b-ja")

#checksums = {}
#with open(checksumfile, "r") as f:
#    lines = f.readlines()
#    for line in lines:
#        tup = line.split()
#        checksums[tup[1]] = os.path.basename(tup[0])

# Create directory if not exists.
os.makedirs(dst_wiki40b_path, exist_ok=True)

offset = 0
n = nfiles

nprocesses = 6

def worker(filepath):

    print(filepath)

    with open(filepath, 'rb') as f:
        indata = f.read()

    dctx = zstandard.ZstdDecompressor()
    dobj = dctx.decompressobj()
    jsonldata = dobj.decompress(indata)

    lines = jsonldata.splitlines()
    del indata

    dst_lines = []

    for line in lines:
        j = json.loads(line)

        j["text"] = text_normalizer.normalize(j["text"])

        dst_lines.append(json.dumps(j, ensure_ascii=False))

        del j

    del lines

    zctx = zstandard.ZstdCompressor(level=zstd_comp_level)
    zfilename = os.path.join(dst_wiki40b_path, os.path.splitext(os.path.basename(filepath))[0] + ".zstd")

    dst_buf = "\n".join(dst_lines)

    # TODO: Use stream
    zcompressed = zctx.compress(bytes(dst_buf, 'utf-8'))

    print("write to ", zfilename)
    of = open(zfilename, 'wb')
    of.write(zcompressed)
    of.close()


    del dst_buf
    del zcompressed
   
    print("done")

        

if len(sys.argv) > 2:
    offset = int(sys.argv[1])
    n = int(sys.argv[2])

    assert offset >= 0
    assert offset < (nfiles+1)
    assert n > 0

assert (offset + n) < (nfiles+1)

inputfiles = []
for i in range(n):
    idx = offset + i
    
    # starts with 0.
    filepath = wiki40b_glob_pattern.format(idx)
    inputfiles.append(filepath)

with concurrent.futures.ProcessPoolExecutor(max_workers=nprocesses) as executor:
    fs = list(tqdm(executor.map(worker, inputfiles), total=len(inputfiles)))

