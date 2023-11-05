# 日本語データセットで追加事前学習

- 日本語データセット
- 日本語トークナイザ

で追加事前学習(incremental pre-training)を行います.

事前に日本語データセットと日本語トークナイザを用意する必要があります.

## Status

* [x] tinyllama 1.1B を incremental pretrain 
* [ ] llama2 7B 

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


### TinyLlama 1.1B

```
$ bash ./run_jp_tinyllama_train.sh
```

最初にデータセットの tokenize や grouping を行います.
結果はキャッシュされます.
CharShu 59B(展開時180GB) では, これらのキャッシュとして概ね 800 GB を必要とします.
tokenize 処理には 2~3 時間ほどかかります.

batch=1 で 20 GB ほどメモリを消費します.

### Flash Attention

transformers に取り込まれた? flash_attn_2 を使います.
ROCm の場合は現状(2023/09/15 時点) MI250(gfx90a) 以外対応していないようので, off にします.

## TODO

* [ ] 学習用にトークナイズなどする処理の別 script 化
* [ ] libtorch or llama.cpp で CPU クラスタで incremental pretrain できるようにする.

## Third party licenses

* lit llama: Apache 2.0 https://github.com/Lightning-AI/lit-llama
