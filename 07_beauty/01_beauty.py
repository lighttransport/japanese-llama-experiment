import glob
import os
import sys

02_clean_basedir = "../data/02_clean_step"

02_clean_info = {
  "cc100ja": { ["cc100ja", "*.jsonl.zstd", "text"] },
  "mc4": { ["mc4", "*.json.zstd", "text"] }
  "oscar": { ["OSCAR2301", "*.jsonl.zstd", "content"]
}


# --

for ds in 02_clean_info:

    ds_folder = ds[0]
    glob_pattern = ds[1]
    text_key = ds[2]

    fs = glob.glob(os.path.join(02_clean_basedir, os.path.join(ds_folder, glob_pattern)))
    for f in fs:
        print(f)

