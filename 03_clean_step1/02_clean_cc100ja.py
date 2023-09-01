# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2023 - Present, Light Transport Entertainment Inc.
#
# filter newline
import sys
import gzip
import json
import os
import signal
#from concurrent.futures import ThreadPoolExecutor
#from multiprocessing import Pool
from pathlib import Path


import tqdm
import zstandard

nfiles = 1001
cc100ja_glob_pattern = "../data/02_clean_step1/cc100ja/cc100-ja.{:05d}.jsonl.zstd"
dst_cc100ja_path = Path("../data/02_clean_step1/cc100ja")

nprocesses = 12

zstd_comp_level = 7

#checksums = {}
#with open(checksumfile, "r") as f:
#    lines = f.readlines()
#    for line in lines:
#        tup = line.split()
#        checksums[tup[1]] = os.path.basename(tup[0])

# Create directory if not exists.
os.makedirs(dst_cc100ja_path, exist_ok=True)

def do_clean(text: str):

    text = text.replace('\n\\n', '\n')
    return text.replace('\\n', '')

def worker(filepath):

    with open(filepath, 'rb') as f:
        indata = f.read()

    basefilename = os.path.basename(filepath)

    #checksum = hashlib.sha256(indata).hexdigest()
    #if checksum == checksums[basefilename]:
    #    print("Checksum OK", filepath)
    #else:
    #    print("Checksum check failed for ", filepath)
    #    return

    dctx = zstandard.ZstdDecompressor()
    dobj = dctx.decompressobj()
    jsonldata = dobj.decompress(indata)

    lines = jsonldata.splitlines()
    del indata

    dst_lines = []

    nlines = len(lines)
    #print(lines)
    for line in lines:
        j = json.loads(line)

        j["text"] = do_clean(j["text"])

        if j["text"]:
            dst_lines.append(json.dumps(j, ensure_ascii=False))

        del j

    del lines


    zctx = zstandard.ZstdCompressor(level=zstd_comp_level)
    zfilename = os.path.join(dst_cc100ja_path, os.path.splitext(os.path.basename(filepath))[0] + ".zstd")

    dst_buf = "\n".join(dst_lines)

    # TODO: Use stream
    zcompressed = zctx.compress(bytes(dst_buf, 'utf-8'))

    print("write to ", zfilename)
    of = open(zfilename, 'wb')
    of.write(zcompressed)
    of.close()


    del dst_buf
    del zcompressed
    

#
# - main
#

if __name__ == '__main__':

    offset = 0
    n = nfiles


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
        filepath = cc100ja_glob_pattern.format(idx)
        inputfiles.append(filepath)

    for filepath in tqdm.tqdm(inputfiles):

        worker(filepath)

