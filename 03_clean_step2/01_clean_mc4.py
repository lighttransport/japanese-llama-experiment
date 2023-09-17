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

nfiles = 1024
mc4_glob_pattern = "../data/01_normalized/mc4/c4-ja.tfrecord-{:05d}-of-01024.json.zstd"

# overwrite
dst_mc4_path = Path("../data/02_clean_step/mc4")

nprocesses = 12

#checksums = {}
#with open(checksumfile, "r") as f:
#    lines = f.readlines()
#    for line in lines:
#        tup = line.split()
#        checksums[tup[1]] = os.path.basename(tup[0])

# Create directory if not exists.
os.makedirs(dst_mc4_path, exist_ok=True)

def worker(filepath):

    import subprocess

    dst_filename = os.path.join(dst_mc4_path, os.path.splitext(os.path.basename(filepath))[0] + ".zstd")

    cmd = ['python', 'clean_mc4_task.py', filepath, dst_filename]

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
        
        # starts with 0.
        filepath = mc4_glob_pattern.format(idx)
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
