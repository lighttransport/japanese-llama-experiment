import json
import zstandard
from pathlib import Path


def load_jsonl_zstd(filename: Path):
  
    with open(filepath, 'rb') as f:
        indata = f.read()

    # TODO: stream decoding.
    dctx = zstandard.ZstdDecompressor()
    dobj = dctx.decompressobj()
    jsonldata = dobj.decompress(indata)

    lines = jsonldata.splitlines()
    del indata

    jsons = []

    for line in lines:
        if len(line) < 1:
            continue

        j = json.loads(line)

        jsons.append(j)


    return jsons
