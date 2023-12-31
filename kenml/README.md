## built model

You can download prebuilt KenLM model here: https://github.com/lighttransport/data-for-japanese-corpus

```
$ git lfs install
$ git clone https://github.com/lighttransport/data-for-japanese-corpus
```

## Procedure

### Char version

### Wakachi-gaki version

Build kenlm: build.txt
Download wikidataset: download_wiki.txt
Prepare wikidataset with Unicode normalization and wakachi-gaki: wakachi_tokenize.py

Shuffle dataset

```
$ shuf wiki-nfkc-wakachi.txt > wiki-nfkc-wakachi-shuffled.txt
```

Train with KenLM

```
$ ./kenlm/build/bin/lmplz --order 4 --discount_fallback < wiki-nfkc-wakachi-shuffled.txt > kenlm_model-wiki-nfkc-wakachi.arpa
```


