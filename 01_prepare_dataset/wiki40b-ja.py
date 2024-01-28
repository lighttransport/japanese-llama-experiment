#
# Split sentences by newline only line
#
import os, sys
import json
import zstandard
from tqdm import tqdm

input_filepath = "../data/00_dataset/wiki40b-ja.jsonl.zstd"
output_dir = "../data/00_dataset/wiki40b-ja/"

os.makedirs(output_dir, exist_ok=True)

indata = open(input_filepath, "rb").read()
dctx = zstandard.ZstdDecompressor()
dobj = dctx.decompressobj()
jsonldata = dobj.decompress(indata)

lines = jsonldata.splitlines()

# roughly 10 ~ 80 MB when compressed.
max_json_items = 1024*64

zstd_comp_level = 5

def write_jsonl_file(jsonlines, zfilename):
    zctx = zstandard.ZstdCompressor(level=zstd_comp_level)

    print("write ", zfilename)
    dst_buf = "\n".join(jsonlines)
    zcompressed = zctx.compress(bytes(dst_buf, 'utf-8'))

    of = open(zfilename, 'wb')
    of.write(zcompressed)
    of.close()


json_items = 0
json_file_idx = 0
sentencelines = []
jsonlines = []
for line in tqdm(lines):

    jsonlines.append(line.decode())

    if len(jsonlines) >= max_json_items:
        zfilename = os.path.join(output_dir, "wiki40b-ja.{:05d}.jsonl.zstd".format(json_file_idx))  

        write_jsonl_file(jsonlines, zfilename)

        jsonlines = []

        json_file_idx += 1

    else:
        # strip "\n"
        sentencelines.append(line.strip())

if len(jsonlines):
    zfilename = os.path.join(output_dir, "wiki40b-ja.{:05d}.jsonl.zstd".format(json_file_idx))  
    write_jsonl_file(jsonlines, zfilename)


