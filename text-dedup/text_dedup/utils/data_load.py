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
            d[k].append(jsons[i][k])


    ds = Dataset.from_dict(d)
    return ds


def save_file(jsons, zfilepath: Path):
    """
    Save json[] to a file.
    """

    zctx = zstandard.ZstdCompressor(level=zstd_comp_level)

    dst_lines = []
    for j in jsons:
        dst_lines.append(json.dumps(j, ensure_ascii=False))

    # TODO: Use stream
    zcompressed = zctx.compress(bytes(dst_buf, 'utf-8'))

    of = open(zfilename, 'wb')
    of.write(zcompressed)
    of.close()

    return True
