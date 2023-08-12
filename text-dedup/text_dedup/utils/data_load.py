# SPDX-License-Identifier: Apache 2.0
# Copyright 2023 - Present: Light Transport Entertainment, Inc.

import os
from pathlib import Path
import json

# Loads jsonl.zstd data
import zstandard

zstd_comp_level = 5 # default = 3


#
# TODO: stream interface
#
def load_file(filepath: Path):
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


    return jsons


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
