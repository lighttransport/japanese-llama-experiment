# SPDX-License-Identifier: Apache 2.0
# Copyright 2023 - Present: Light Transport Entertainment, Inc.

import os
from pathlib import Path
import json

# Loads jsonl.zstd data
import zstandard

zstd_comp_level = 5 # default = 3

# HuggingFace datasets
import datasets
from datasets import Dataset

import glob

def load_json_files(filepath_pattern, offset, nfiles):
    """
    filepath_pattern: path with glob pattern("/path/to/cc100-ja-{:05d}.jsonl.zstd")
    """

    jsons = []

    for i in range(nfiles):
        idx = offset + i
        fname = filepath_pattern.format(idx)

        if not os.path.exists(fname):
            print("file not found. skipping: ", fname)

        dctx = zstandard.ZstdDecompressor()
        dobj = dctx.decompressobj()

        print("loading ", fname)
        with open(fname, 'rb') as f:
            indata = f.read()

        jsonldata = dobj.decompress(indata)

        lines = jsonldata.splitlines()
        del indata

        for line in lines:
            j = json.loads(line)
            jsons.append(j)

    #
    # Simple row to column conversion
    #
    keys = jsons[0].keys()

    d = {}
    for k in keys:
        d[k] = []

        for i in range(len(jsons)):
            s = jsons[i][k]
            if isinstance(s, str):
                # unescape \\n
                s = s.replace('\\n', '\n')
                pass
            d[k].append(s)


    ds = Dataset.from_dict(d)
    return ds

def load_file(filepath: Path):
    """
    load jsonl+zstd file and return as huggingface datasets
    """

    dctx = zstandard.ZstdDecompressor()
    dobj = dctx.decompressobj()

    with open(filepath, 'rb') as f:
        indata = f.read()

    jsonldata = dobj.decompress(indata)

    lines = jsonldata.splitlines()
    del indata

    jsons = []
    for line in lines:
        j = json.loads(line)
        jsons.append(j)

    #
    # Simple row to column conversion
    #
    keys = jsons[0].keys()

    d = {}
    for k in keys:
        d[k] = []

        for i in range(len(jsons)):
            s = jsons[i][k]
            if isinstance(s, str):
                # unescape \\n
                s = s.replace('\\n', '\n')
            d[k].append(s)


    ds = Dataset.from_dict(d)
    return ds


def save_file(buf, zfilepath: Path):
    """
    Save json[] to a file.
    buf: bytes
    """

    zctx = zstandard.ZstdCompressor(level=zstd_comp_level)

    #dst_lines = []
    #for j in jsons:
    #    dst_lines.append(json.dumps(j, ensure_ascii=False))

    # TODO: Use stream
    #zcompressed = zctx.compress(bytes(buf, 'utf-8'))

    zcompressed = zctx.compress(buf)

    of = open(zfilepath, 'wb')
    of.write(zcompressed)
    of.close()

    return True
