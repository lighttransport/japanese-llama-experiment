import os

from torch.utils.data import DataLoader
from lit_llama.packed_dataset import PackedDataset, CombinedDataset
#from itertools import chain

#from datasets import load_dataset
from transformers import LlamaTokenizer

import load_jsonl_dataset

dataset_dir="lighttransport/Japanese-CharShu-59B"
jp_tokenizer_path = "../data/merged_tokenizer_hf/"
cache_path= "mycache"
cache_dir = "temp_data_cache_dir_prepare"
block_size = 1-24
num_proc = 8

os.makedirs(cache_path, exist_ok=True)
os.makedirs(cache_dir, exist_ok=True)

tokenizer = LlamaTokenizer.from_pretrained(jp_tokenizer_path)

raw_dataset = load_dataset(dataset_dir)
print(raw_dataset)

for split_name in raw_dataset.keys():
    # only use 'text' columns
    raw_dataset[split_name] = raw_dataset[split_name].select_columns(['text'])

print(raw_dataset)

def tokenize_text(example):
    output = tokenizer.tokenize(example["text"])
    return output

def group_texts(examples):
    # Concatenate all texts.
    concatenated_examples = {k: list(chain(*examples[k])) for k in examples.keys()}
    total_length = len(concatenated_examples[list(examples.keys())[0]])
    # We drop the small remainder, we could add padding if the model supported it instead of this drop, you can
    # customize this part to your needs.
    if total_length >= block_size:
        total_length = (total_length // block_size) * block_size
    # Split by chunks of max_len.
    result = {
        k: [t[i : i + block_size] for i in range(0, total_length, block_size)]
        for k, t in concatenated_examples.items()
    }
    result["labels"] = result["input_ids"].copy()
    return result

#tokenized_datasets = raw_dataset.map(tokenize_text,
#    num_proc=num_proc,
#    remove_columns="text",
#    load_from_cache_file=True,
#    keep_in_memory=False,
#    cache_file_names = {k: os.path.join(cache_dir, 'tokenized.arrow') for k in raw_dataset},
#    desc="Running tokenizer on dataset",
#    batched=True)
#print(tokenized_datasets)
#
#grouped_datasets = tokenized_dataset.map(
#    group_texts,
#    batched=True,
#    num_proc=num_proc,
#    load_from_cache_file=True,
#    keep_in_memory=False,
#    cache_file_names = {k: os.path.join(cache_dir, 'grouped.arrow') for k in tokenized_dataset},
#    desc=f"Grouping texts in chunks of {block_size}",
#)
#processed_dataset = grouped_datasets
#processed_dataset.save_to_disk(cache_path)

