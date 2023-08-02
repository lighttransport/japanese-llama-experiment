import json
from datasets import load_dataset

import zstandard

dataset = load_dataset("range3/wiki40b-ja")

js = []
for item in dataset['train']:
    j = json.dumps(item, ensure_ascii=False)

    js.append(j)

zstd_comp_level = 4
zctx = zstandard.ZstdCompressor(level=zstd_comp_level)
zfilename = "wiki40b-ja.jsonl.zstd"

dst_buf = "\n".join(js)

# TODO: Use stream
zcompressed = zctx.compress(bytes(dst_buf, 'utf-8'))

print("write to ", zfilename)
of = open(zfilename, 'wb')
of.write(zcompressed)
of.close()

