import os
from pathlib import Path
import glob
import subprocess

input_dir = "WARC"
zstd_compression = 4

def conv(filename):

    zstd_file = os.path.join(Path(filename).parent, Path(filename).stem + ".zstd")
        
    cmd = ["gunzip", "-c", filename, "|", "zstd", "-{}".format(zstd_compression), "-o", zstd_file]
    print(cmd)

    try:
        retcode = subprocess.call(" ".join(cmd), shell=True)
        if retcode != 0:
            print("Failed: ", cmd, " Skipping...")
        else:
            # rm .gz file
            subprocess.call("rm {}".format(filename), shell=True)
            
    except OSError as e:
        print("Failed to execute: ", cmd, " Skipping...")


files = glob.glob(os.path.join(input_dir, "*.warc.gz"))

for f in files:
    conv(f)





