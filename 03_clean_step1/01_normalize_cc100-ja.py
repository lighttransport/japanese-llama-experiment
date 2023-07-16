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
#import text_normalizer

zstd_comp_level = 4 # default = 3

nfiles = 1001
cc100ja_glob_pattern = "../data/01_normalized/cc100ja/cc100-ja.{:05d}.jsonl.zstd"

# TODO: checksum
#checksumfile = "/mnt/disk01/cc100/checksum.sha256"
dst_cc100ja_path = Path("../data/02_clean_step1/cc100ja")

#checksums = {}
#with open(checksumfile, "r") as f:
#    lines = f.readlines()
#    for line in lines:
#        tup = line.split()
#        checksums[tup[1]] = os.path.basename(tup[0])

# Create directory if not exists.
os.makedirs(dst_cc100ja_path, exist_ok=True)

#files = glob.glob(mc4_glob_pattern)

nprocesses = 8

def do_clean(text: str):

    # 1. remove if the text contains whitespace 
    # 2. remove sentence if it ends with "...", "," or no punctuation.

    if ' ' in text:
        return None

    if '　' in text: # zenkaku-space
        return None

    sentences = text.split('\\n') # split by escaped '\n'
    results = []
    for sent in sentences:
        if sent.endswith("..."):
            continue
        elif sent.endswith("."):
            # ends with period(after normalization, '。' was replaced to '.')
            pass
        elif sent.endswith(")"):
            # FIXME: May be ascii kaomoji :-)
            pass
        elif sent.endswith("!"):
            pass
        elif sent.endswith("?"):
            pass
        elif sent.endswith(","):
            continue
        else:
            # assume sentence is broken.
            # TODO: Do nlp analysys 
            continue

        results.append(sent)

    return "\\n".join(results)
        

def worker(filepath):

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

    #print(lines)
    for line in lines:
        j = json.loads(line)

        # Simple version NFKC
        #j["content"] = unicodedata.normalize('NFKC', j["content"])

        # cc_net compatible version. apply NFKC normalization also. 
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
   
    print("done")

        
#
# - main
#
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
    filepath = cc100ja_glob_pattern.format(idx+1)
    inputfiles.append(filepath)

#with concurrent.futures.ProcessPoolExecutor(max_workers=nprocesses) as executor:
#    executor.map(worker, inputfiles)

worker(inputfiles[0])
