# 日本語データセットで追加事前学習

- 日本語データセット
- 日本語トークナイザ

で追加事前学習(incremental pre-training)を行います.

事前に日本語データセットと日本語トークナイザを用意する必要があります.

## Status

W.I.P.

## Requirements

* Python 3.10+
* pytorch
* pytorch lightning


## Setup

```
$ conda create -n jp-llama-pretrain python=3.10
$ conda activate jp-llama-pretrain
$ python -m pip install -r requirements.txt
```

## Training

### Flash Attention

ROCm の場合は現状(2023/09/15 時点) MI250(gfx90a) 以外対応していないようので, off にします.

T.B.W.

## TODO

* [ ] libtorch or llama.cpp で CPU クラスタで incremental pretrain できるようにする.

## Third party licenses

* lit llama: Apache 2.0 https://github.com/Lightning-AI/lit-llama
