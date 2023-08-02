# Japanese LLaMa experiment.

Japanese version [README-ja.md](README-ja.md)

## Status

* [x] Japanese dataset pre-cleaning
* [ ] Japanese dataset quality filtering & dedup
* [ ] Incremental pre-training
* [ ] Fine-tuning with Japanese finetuning dataset.

## Requirements

* (Mini)conda
* Python 3.10+
  * Python 3.8+ may work.

### To prepare Japanese dataset

* KenLM

Build and install python module.

```
$ sudo apt install build-essential cmake libboost-system-dev libboost-thread-dev libboost-program-options-dev libboost-test-dev libeigen3-dev zlib1g-dev libbz2-dev liblzma-dev

$ git clone https://github.com/kpu/kenlm
$ cd kenlm
$ python setup.py bdist_wheel
$ python -m pip install -U dist/kenlm*.whl
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

We need to create two conda environment, since `spacy-transformers` module(used in `ginza` module) requires older `transformers` version, which does not support Llama class(fail to import LlamaTokenizer from transformers)


```
$ conda create -n jp-llama-experiment python=3.10
$ conda activate jp-llama-experiment
$ python -m pip install -r requirements.txt
```

```
$ conda deactivate
$ conda create -n jp-llama-experiment-nlp python=3.10
$ conda activate jp-llama-experiment-nlp
$ python -m pip install -r requirements-ja-nlp.txt
```

## Steps

0. Download datasets.
1. Run dataset cleaner
1. Train Japanese Tokeniezr
2. Merge Japanese Tokenizer into LLaMa Tokenizer
3. LoRA incremental training using Japanese Tokenizer
4. Finetune with Japanese dataset(e.g. Alpaca)

### Download datasets

This is a required stop to train Tokenier, build KenLM model, etc.

* cc100ja
* mc4 ja
* OSCAR2301 ja
* wiki40b/ja

See `00_download_dataset` for details.

### Run dataset cleaner

* [x] `01_prepare_dataset`
* [x] `02_normalize/`
* [x] `03_clean_step1/`
* [ ] `04_lm_scoring/`
* [ ] `05_dedup/`

### Train Japanese Tokenizer

W.I.P.

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

T.B.W.

## License

MIT license unless licensing terms is not explicitly denoted.
Some scripts are licensed under Apache 2.0 or BSD.

### Third party licenses

Chinese LLaMa: Apache 2.0: https://github.com/ymcui/Chinese-LLaMA-Alpaca
cc_net: MIT License https://github.com/facebookresearch/cc_net
