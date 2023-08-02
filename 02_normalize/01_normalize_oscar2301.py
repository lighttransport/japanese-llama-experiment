# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2023 - Present, Light Transport Entertainment Inc.
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

zstd_comp_level = 4 # default = 3

nfiles = 123
oscar_glob_pattern = "/mnt/disk01/OSCAR-2301/ja_meta/ja_meta_part_{}.jsonl.zst"
checksumfile = "/mnt/disk01/OSCAR-2301/ja_meta/checksum.sha256"
dst_oscar_path = Path("../data/01_normalized/OSCAR-2301")

checksums = {}
with open(checksumfile, "r") as f:
    lines = f.readlines()
    for line in lines:
        tup = line.split()
        checksums[tup[1]] = os.path.basename(tup[0])

# Create directory if not exists.
os.makedirs(dst_oscar_path, exist_ok=True)

#files = glob.glob(mc4_glob_pattern)

offset = 0
n = nfiles

nprocesses = 6

def worker(filepath):

    print(filepath)

    with open(filepath, 'rb') as f:
        indata = f.read()

    checksum = hashlib.sha256(indata).hexdigest()

    basefilename = os.path.basename(filepath)
    if checksum == checksums[basefilename]:
        print("Checksum OK", filepath)
    else:
        print("Checksum check failed for ", filepath)
        return

    dctx = zstandard.ZstdDecompressor()
    dobj = dctx.decompressobj()
    jsonldata = dobj.decompress(indata)

    lines = jsonldata.splitlines()
    del indata

    dst_lines = []

    for line in lines:
        j = json.loads(line)

        # Simple version NFKC
        #j["content"] = unicodedata.normalize('NFKC', j["content"])

        # cc_net compatible version. apply NFKC normalization also. 
        j["content"] = text_normalizer.normalize(j["content"])

        dst_lines.append(json.dumps(j, ensure_ascii=False))

        del j

    del lines

    zctx = zstandard.ZstdCompressor(level=zstd_comp_level)
    zfilename = os.path.join(dst_oscar_path, os.path.splitext(os.path.basename(filepath))[0] + ".zstd")

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
    
    # starts with 1.
    filepath = oscar_glob_pattern.format(idx+1)
    inputfiles.append(filepath)

#with Pool(nprocesses) as p:
#    p.map(worker, inputfiles)

with concurrent.futures.ProcessPoolExecutor(max_workers=nprocesses) as executor:
    executor.map(worker, inputfiles)



    

    
