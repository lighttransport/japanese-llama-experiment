#!/bin/bash

src_dir=../data/02_clean_step/OSCAR-2301/
out_dir=../data/05_minhash/
../cpp/build/cpp_proc minhash ${src_dir} ${out_dir} content
