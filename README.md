# Japanese LLaMa experiment.

## Requirements

* (Mini)conda
* Python 3.10+
  * Python 3.8+ may work.

### To prepare Japanese dataset

* KenLM

```
$ sudo apt install build-essential cmake libboost-system-dev libboost-thread-dev libboost-program-options-dev libboost-test-dev libeigen3-dev zlib1g-dev libbz2-dev liblzma-dev

$ git clone https://github.com/kpu/kenlm
$ cd kenlm
$ mkdir build
$ cd build
$ cmake ..
$ make
```

* sentencepiece

```
$ sudo apt install sentencepiece
```

* Download SentencePiece and KenLM pretrained model(for `ja` language)

```
$ bash download_lm.sh
```

## Install

Setup python environment using conda.

```
$ conda create -n jp-llama-experiment python=3.10
$ conda activate jp-llama-experiment
```

Install modules.

```
$ python -m pip install -r requirements.txt
```

## Steps

1. Train Japanese Tokeniezr
2. Merge Japanese Tokenizer into LLaMa Tokenizer
3. LoRA incremental training using Japanese Tokenizer
4. Finetune with Japanese dataset(e.g. Alpaca)

### Train Japanese Tokenizer

See 

cc100 ja で日本語 tokenizer を huggingface tokenizers で train するメモ
https://zenn.dev/syoyo/articles/8647ae42a3be63

for details(in Japanese)

Train Japanese Tokenizer from cc100 ja.

It will download 40 GB of cc100 ja datset(75 GB uncompressed).

`train_jp_tokenizer.py`

128 GB CPU memory is required to train Japanese Tokenizer.
After downloading

### Merge Japanese Tokenizer vocab into LLaMa tokenizer 

T.B.W.

### Incremental training using Japanese Tokenizer.

This step take a time to train.

T.B.W.


### Finetune with Japanese dataset(e.g. Alpaca)

T.B.W.

## TODO

* [ ] Normalize text(e.g. convert hankaku-kana to zenkaku-kana) when training Japanese Tokenizer?

## License

MIT license unless licensing terms is explicitly denoted.
Some scripts are licensed under Apache 2.0.

### Third party licenses

Chinese LLaMa: Apache 2.0: https://github.com/ymcui/Chinese-LLaMA-Alpaca
cc_net: MIT License https://github.com/facebookresearch/cc_net
