import zstandard

def load_jsonl_zstd(filepath):
    dctx = zstandard.ZstdDecompressor()
    js = []
    with open(filepath) as f:
        decompressed = dctx.decompress(f.read())

        lines = decompressed.split('\n')

        for line in lines:
            j = json.loads(line)
            js.append(j)


    return js
