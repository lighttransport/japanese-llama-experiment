import sys
import gzip
import json
import unicodedata
import glob
import os
from pathlib import Path

import zstandard
import text_normalizer

zstd_comp_level = 4 # default = 3

mc4_glob_pattern = "/mnt/disk01/work/c4/multilingual/c4-ja.tfrecord-{:05d}-of-01024.json.gz"
dst_mc4_path = Path("../data/01_normalized/mc4")

# Create directory if not exists.
os.makedirs(dst_mc4_path, exist_ok=True)

#files = glob.glob(mc4_glob_pattern)

offset = 0
n = 10

if len(sys.argv) > 2:
    offset = int(sys.argv[1])
    n = int(sys.argv[2])

    assert offset >= 0
    assert offset < 1024
    assert n > 0

assert (offset + n) < 1024

for i in range(n):
    idx = offset + i
    
    filepath = mc4_glob_pattern.format(idx)
    print("Processing ", filepath)
    f = gzip.open(filepath, 'rb')
    lines=f.readlines()
    f.close()

    dst_lines = []

    for line in lines:
        j = json.loads(line)

        # Simple version NFKC
        #j["text"] = unicodedata.normalize('NFKC', j["text"])

        # cc_net compatible version. apply NFKC normalization also. 
        j["text"] = text_normalizer.normalize(j["text"])

        dst_lines.append(json.dumps(j, ensure_ascii=False))

    #print(lines[0])

    zctx = zstandard.ZstdCompressor(level=zstd_comp_level)
    zfilename = os.path.join(dst_mc4_path, os.path.splitext(os.path.basename(filepath))[0] + ".zstd")

    dst_buf = "\n".join(dst_lines)

    # TODO: Use stream

    zcompressed = zctx.compress(bytes(dst_buf, 'utf-8'))

    print("write to ", zfilename)
    of = open(zfilename, 'wb')
    of.write(zcompressed)
    of.close()

    
