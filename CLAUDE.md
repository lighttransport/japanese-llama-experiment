# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a Japanese LLaMa experiment project for creating Japanese language models through:
1. Japanese dataset preprocessing (cleaning, normalization, deduplication)
2. Japanese tokenizer training and merging with LLaMa tokenizer
3. Incremental pre-training on Japanese datasets
4. Fine-tuning with Japanese instruction datasets

## Development Environment Setup

Two separate conda environments are required due to dependency conflicts:

**Main environment** (for tokenization, training):
```bash
conda create -n jp-llama-experiment python=3.10
conda activate jp-llama-experiment
python -m pip install -r requirements.txt
```

**NLP environment** (for text processing with ginza):
```bash
conda create -n jp-llama-experiment-nlp python=3.10
conda activate jp-llama-experiment-nlp
python -m pip install -r requirements-ja-nlp.txt
```

The split is necessary because `spacy-transformers` (used by `ginza`) requires an older `transformers` version incompatible with LLaMa classes.

## Building C++ Components

The `cpp/` directory contains `cpp_proc`, a high-performance C++ tool for dataset processing tasks.

**Basic build**:
```bash
cd cpp
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make
```

**Build options**:
- `-DCPPPROC_WITH_PYTHON=ON` - Embed Python interpreter
- `-DCPPPROC_WITH_IMAGING=ON` - Image loading/writing for LLaVa datasets (default: ON)
- `-DCPPPROC_WITH_JDEPP=ON` - Japanese dependency parsing with J.DepP (default: ON)
- `-DCPPPROC_WITH_LLAMACPP=ON` - Include llama.cpp for tokenization (default: ON)

The `cpp_proc` tool supports these tasks:
- `build` - Build tokenizer or other data structures
- `dedup` - Deduplication processing
- `search` - Search operations

## Dataset Processing Pipeline

The numbered directories represent a sequential data processing pipeline:

### 00_download_dataset
Download Japanese datasets: cc100ja, mc4 ja, OSCAR2301 ja, wiki40b/ja

### 01_prepare_dataset
Initial dataset preparation scripts for various sources

### 02_normalize
Text normalization using `text_normalizer.py` (based on cc_net with Japanese-specific modifications)
- Scripts: `01_normalize_*.py` for different datasets
- Run all: `bash run-all.sh`

### 03_clean_step1 through 03_clean_step4_kanji
Multi-stage cleaning pipeline:
- Step 1: Basic cleaning and filtering
- Step 2: Sentence segmentation using bunkai
- Step 3: Line-wise filtering for Japanese text
- Step 4: Kanji-specific filtering

Key modules:
- `jp_sentence_check.py` - Japanese sentence validation
- `url_filtering.py` - URL filtering
- `ascii_filtering.py` - ASCII content filtering

### 04_lm_scoring
Quality scoring using KenLM language models. Requires:
- KenLM built and installed
- sentencepiece installed
- Pretrained model files in `../data/lm_sp/`
- Run `bash download_lm.sh` to download models

Input must be sentence-segmented (one sentence per line from step 2).

### 05_minhash
MinHash computation for fuzzy deduplication

### 06_dedup
Deduplication processing using MinHash results from step 5.
Reads MinHash JSONL files and outputs JSONL with `duplicated` parameter added.

## Tokenizer Training and Merging

Located in `train_tokenizer/`:

1. **Train Japanese tokenizer**: `python train_jp_tokenizer.py`
   - Trains on cc100 ja dataset (~40 GB download, 75 GB uncompressed)
   - Requires 128 GB CPU memory

2. **Merge tokenizers**: `python merge_tokenizers.py`
   - Merges Japanese tokenizer vocab into LLaMa tokenizer

## Incremental Pre-training

Located in `10_incremental_pretrain/`:

**Requirements**:
```bash
conda create -n jp-llama-pretrain python=3.10
conda activate jp-llama-pretrain
python -m pip install -r requirements.txt
```

**Training TinyLlama 1.1B**:
```bash
bash ./run_jp_tinyllama_train.sh
```

- Initial dataset tokenization and grouping is cached (~800 GB for CharShu 59B dataset)
- Tokenization takes 2-3 hours
- Memory usage: ~20 GB with batch=1
- Supports flash_attn_2 (ROCm limited to MI250/gfx90a as of 2023/09/15)

**Publishing checkpoints**:
```bash
python publish_checkpoint.py
```
Merges checkpoints and resizes model embeddings to match tokenizer vocab for compatibility with tools like llama.cpp.

## Architecture Notes

### Dual-Environment Design
The project uses two conda environments to handle dependency conflicts between modern transformers (needed for LLaMa) and older versions (needed for ginza Japanese NLP tools). When working on code:
- Use `jp-llama-experiment` for anything involving LLaMa models, tokenizers, or training
- Use `jp-llama-experiment-nlp` for Japanese text processing with ginza

### C++ Processing Tool (cpp_proc)
The `cpp/` directory contains a high-performance data processing tool with:
- JSON processing using simdjson and nlohmann/json
- Compression: zstd, lz4
- Japanese tokenization: jagger (morphological analysis), tinysegmenter
- Deduplication: suffix arrays (libsais), MinHash
- Optional llama.cpp integration for tokenization
- Unicode normalization via utf8proc

Key third-party libraries are vendored as single-header or amalgamated sources.

### Dataset Format
Datasets flow through the pipeline as JSONL (JSON Lines) format, typically with zstd compression. Each processing stage adds metadata fields (e.g., quality scores, dedup flags) while preserving the text content.

## Common Workflows

**Running the full preprocessing pipeline**:
1. Download datasets (00_download_dataset)
2. Normalize (02_normalize/run-all.sh)
3. Clean stages 1-4 (03_clean_step*/run-all.sh)
4. Score with KenLM (04_lm_scoring)
5. Compute MinHash (05_minhash)
6. Deduplicate (06_dedup)

**Training a model from scratch**:
1. Complete preprocessing pipeline above
2. Train Japanese tokenizer (train_tokenizer/)
3. Merge with LLaMa tokenizer
4. Incremental pre-train (10_incremental_pretrain/)
5. Optional fine-tuning (20_finetune/ or 20_sft/)
