#!/bin/bash

inout_dir=/mnt/disk01/work/japanese-dataset-cleaned-experiment/05_minhash/
out_dir=/mnt/disk01/work/japanese-dataset-cleaned/06_dedup/
../cpp/build/cpp_proc dedup ${inout_dir} ${out_dir}
