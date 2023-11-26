#
# Split sentences by newline only line
#
import os, sys
import json
import zstandard

input_filepath = "/mnt/disk01/cc100/ja.txt"
output_dir = "/mnt/disk01/cc100/"

os.makedirs(output_dir, exist_ok=True)

f = open(input_filepath, "r", encoding="utf-8")

line = f.readline()

# roughly 10 ~ 80 MB when compressed.
max_json_items = 1024*64

zstd_comp_level = 5

json_items = 0
json_file_idx = 0
idx = 0
sentencelines = []
jsonlines = []
while line:

    #if idx > 1024*30:
    #    print("jsonlines", len(jsonlines))
    #    break

    if len(line) == 1 and line[0] == "\n":
        # Create sentence
        if len(sentencelines) > 0:

            d = {}
            # No newline escape required
            # (json.dumps handles it)
            d["text"] = "\n".join(sentencelines)
            d["id"] = idx
            
            j = json.dumps(d, ensure_ascii=False)
            #print(j)
        
            jsonlines.append(j)
            sentencelines = []

            if len(jsonlines) >= max_json_items:
                # serialize
                zctx = zstandard.ZstdCompressor(level=zstd_comp_level)
                zfilename = os.path.join(output_dir, "cc100-ja.{:05d}.jsonl.zstd".format(json_file_idx))  

                print("write ", zfilename)
                dst_buf = "\n".join(jsonlines)
                zcompressed = zctx.compress(bytes(dst_buf, 'utf-8'))

                of = open(zfilename, 'wb')
                of.write(zcompressed)
                of.close()

                jsonlines = []

                json_file_idx += 1
        
            line = f.readline()
            continue

    else:
        # strip "\n"
        sentencelines.append(line.strip())

    line = f.readline()

    idx += 1 

print("max idx", idx)
