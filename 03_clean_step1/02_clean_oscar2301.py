# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2023 - Present, Light Transport Entertainment Inc.
#
# unescape newline
#
import sys
import gzip
import json
import unicodedata
import glob
import os
import hashlib
import signal
import concurrent.futures
#from multiprocessing import Pool
from pathlib import Path

import zstandard
#import text_normalizer
import url_filtering
import ascii_filtering
import clean_text

zstd_comp_level = 4 # default = 3

nfiles = 123

# overwrite 01_clean_oscar2301.py's result
oscar_glob_pattern = "../data/02_clean_step1/OSCAR-2301/ja_meta_part_{}.jsonl.zstd"

# TODO: checksum
#checksumfile = "/mnt/disk01/cc100/checksum.sha256"
dst_oscar_path = Path("../data/02_clean_step1/OSCAR-2301")

#checksums = {}
#with open(checksumfile, "r") as f:
#    lines = f.readlines()
#    for line in lines:
#        tup = line.split()
#        checksums[tup[1]] = os.path.basename(tup[0])

# Create directory if not exists.
os.makedirs(dst_oscar_path, exist_ok=True)

#files = glob.glob(oscar_glob_pattern)

nprocesses = 8

def count_whitespaces(text):
    c = 0
    for i in text:
        if i == " ":
            c += 1

    return c


interrupted = False


def worker(filepath):

    global interrupted

    if interrupted:
        return "Canceled"

    print(filepath)

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

        j["content"] = j["content"].replace("\\n", "\n")

        if j["content"]:
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
        
        # starts with 1.
        filepath = oscar_glob_pattern.format(idx+1)
        inputfiles.append(filepath)

    ## test
    #worker(inputfiles[0])
    #sys.exit(-1)

    def handler(signum, frame):
        global interrupted

        interrupted = True

        # Gracefull shutfown
        print('Signal handler called with signal', signum)
        #sys.exit(-1)

    signal.signal(signal.SIGINT, handler)

    with concurrent.futures.ProcessPoolExecutor(max_workers=nprocesses) as executor:
        try:
            results = executor.map(worker, inputfiles)
            for res in results:
                print("DONE")

        except Exception as exc:
            print(exc)
            executor.shutdown(wait=False, cancel_futures=True)

