##!/bin/bash

./TinySegmenterMaker/extract < xaa > features.txt
./TinySegmenterMaker/train -m 12 -t 0.001 -n 10000 features.txt wiki-102400.model
./TinySegmenterMaker/segment wiki-102400.model < TinySegmenterMaker/test/input.txt 

