#!/bin/bash

lang=ja

wget -c  -P data/lm_sp http://dl.fbaipublicfiles.com/cc_net/lm/${lang}.arpa.bin
wget -c  -P data/lm_sp http://dl.fbaipublicfiles.com/cc_net/lm/${lang}.sp.model
