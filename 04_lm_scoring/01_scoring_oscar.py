# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2023 - Present, Light Transport Entertainment Inc.
import sys
import gzip
import json
import os
import signal
from concurrent.futures import ThreadPoolExecutor
#from multiprocessing import Pool
from pathlib import Path

import tqdm

nfiles = 123
oscar_glob_pattern = "../data/02_clean_step/OSCAR-2301/ja_meta_part_{}.jsonl.zstd"

# only write score value
dst_oscar_path = Path("../data/04_lm_scoring/OSCAR-2301")

nprocesses = 12

#checksums = {}
#with open(checksumfile, "r") as f:
#    lines = f.readlines()
#    for line in lines:
#        tup = line.split()
#        checksums[tup[1]] = os.path.basename(tup[0])

# Create directory if not exists.
os.makedirs(dst_oscar_path, exist_ok=True)


# cc_100
lm_model_filepath = "data/lm_sp/ja.arpa.bin"
sp_model_filepath = "data/lm_sp/ja.sp.model"

def worker(filepath):

    import subprocess

    dst_filename = os.path.join(dst_oscar_path, os.path.splitext(os.path.basename(filepath))[0] + ".zstd")

    text_key = "content" # document key in jsonl
    cmd = ['python', 'scoring_task.py', lm_model_filepath, sp_model_filepath, filepath, dst_filename, text_key ]

    p = subprocess.run(cmd)
    if p.returncode != 0:
        print(p.returncode)
    

#
# - main
#

if __name__ == '__main__':

    offset = 0
    n = nfiles


    if len(sys.argv) > 2:
        offset = int(sys.argv[1])
        n = int(sys.argv[2])

        assert offset >= 0
        assert offset < (nfiles+1)
        assert n > 0

    assert (offset + n) < (nfiles+1)

    inputfiles = []
    for i in range(n):
        idx = offset + i
        
        # starts with 1.
        filepath = oscar_glob_pattern.format(idx+1)
        inputfiles.append(filepath)

    with ThreadPoolExecutor(max_workers=nprocesses) as pool:
        with tqdm.tqdm(total=len(inputfiles)) as progress:
            futures = []

            for f in inputfiles:
                future = pool.submit(worker, f)
                future.add_done_callback(lambda p: progress.update())
                futures.append(future)

            results = []
            
            for f in futures:
                res = f.result()
                results.append(res)
