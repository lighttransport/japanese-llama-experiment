#!/bin/bash

src_dir=../data/02_clean_step/mc4/
out_dir=../data/05_minhash/
../cpp/build/cpp_proc minhash ${src_dir} ${out_dir}
