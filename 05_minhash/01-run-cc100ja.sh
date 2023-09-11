#!/bin/bash

src_dir=/mnt/disk01/work/japanese-dataset-cleaned/02_clean_step1/cc100ja/
out_dir=/mnt/disk01/work/japanese-dataset-cleaned/05_minhash/
../cpp/build/cpp_proc minhash ${src_dir} ${out_dir}
