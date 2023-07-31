# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2023 - Present, Light Transport Entertainment Inc.
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

import ginza
import zstandard
#import text_normalizer
import url_filtering
import ascii_filtering
import clean_text

zstd_comp_level = 4 # default = 3

nfiles = 1024
mc4_glob_pattern = "../data/01_normalized/mc4/c4-ja.tfrecord-{:05d}-of-01024.json.zstd"

# TODO: checksum
#checksumfile = "/mnt/disk01/cc100/checksum.sha256"
dst_mc4_path = Path("../data/02_clean_step1/mc4")

#checksums = {}
#with open(checksumfile, "r") as f:
#    lines = f.readlines()
#    for line in lines:
#        tup = line.split()
#        checksums[tup[1]] = os.path.basename(tup[0])

# Create directory if not exists.
os.makedirs(dst_mc4_path, exist_ok=True)

#files = glob.glob(mc4_glob_pattern)

nprocesses = 8

def char_is_hiragana(c):
    return u'\u3040' <= c <= u'\u309F'

def contains_hiragana(s):
    return any(char_is_hiragana(c) for c in s)

def count_whitespaces(text):
    c = 0
    for i in text:
        if i == " ":
            c += 1

    return c


def do_clean(text: str):

    ws_threshold = 1

    # 1. skip text if it does not contain any hiragana.
    if not contains_hiragana(text):
        return None

    # 2. remove sentence if it ends with "...", "," or no punctuation.
    sentences = text.split('\n') # split by '\n'
    results = []
    for sent in sentences:
        # 1. remove if the sentence contains some whitespaces
        if count_whitespaces(sent) >= ws_threshold:
            continue
        elif sent.endswith("..."):
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
        elif sent.endswith("\""):
            pass
        elif sent.endswith("'"):
            pass
        elif sent.endswith(","):
            continue
        else:
            # assume sentence is broken.
            # TODO: Do nlp analysys 
            continue

        results.append(sent)

    return "\\n".join(results)
        

# global
interrupted = False

def worker(filepath):

    global interrupted

    if interrupted:
        return "Cancel " + filepath

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

        # url filtering
        if url_filtering.filter_url(j["url"]):
            continue

        if ascii_filtering.filter_ascii(j["text"]):
            continue

        # Simple version NFKC
        #j["content"] = unicodedata.normalize('NFKC', j["content"])

        # cc_net compatible version. apply NFKC normalization also. 
        j["text"] = do_clean(j["text"])

        if j["text"]:
            dst_lines.append(json.dumps(j, ensure_ascii=False))

        del j

    del lines


    zctx = zstandard.ZstdCompressor(level=zstd_comp_level)
    zfilename = os.path.join(dst_mc4_path, os.path.splitext(os.path.basename(filepath))[0] + ".zstd")

    dst_buf = "\n".join(dst_lines)

    # TODO: Use stream
    zcompressed = zctx.compress(bytes(dst_buf, 'utf-8'))

    print("write to ", zfilename)
    of = open(zfilename, 'wb')
    of.write(zcompressed)
    of.close()


    del dst_buf
    del zcompressed
   
    return "  {} => {}".format(nlines, len(dst_lines))
        

#
# - main
#

if __name__ == "__main__":
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
        filepath = mc4_glob_pattern.format(idx)
        inputfiles.append(filepath)

    ### test
    #res = worker(inputfiles[0])
    #print(res)
    #sys.exit(-1)

    def handler(signum, frame):

        global interrupted

        # Gracefull shutfown
        print('Signal handler called with signal', signum)
        #sys.exit(-1)

        interrupted = True

    signal.signal(signal.SIGINT, handler)

    with concurrent.futures.ProcessPoolExecutor(max_workers=nprocesses) as executor:

        try:
            results = executor.map(worker, inputfiles)

            for res in results:
                print(res)

        except Exception as exc:
            print(exc)
            executor.shutdown(wait=True, cancel_futures=True)
