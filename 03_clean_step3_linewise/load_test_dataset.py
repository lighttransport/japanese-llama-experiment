import os
import json
import glob
import zstandard

def load_test_dataset(path):
    fs = glob.glob(os.path.join(path, "**/*.jsonl.zstd"), recursive=True)
    fs += glob.glob(os.path.join(path, "*.jsonl.zst"))

    js = []

    for filepath in fs:
        print("reading ... ", filepath)

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
            js.append(j)

    return js
