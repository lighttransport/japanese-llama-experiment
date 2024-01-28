import sys

# import ../common_util/ modules
sys.path.append("..")
from common_util import load_jsonl_zstd

jsons = load_jsonl_zstd.load("../data/00_dataset/cc100/cc100-ja.00000.jsonl.zstd")

import concurrent.futures
from multiprocessing import cpu_count

from tqdm import tqdm
import spacy

model_name = 'ja_core_news_md'
#model_name = 'ja_ginza'
# NOTE: ja_ginza_electra hands in Python multiprocessing environment
#model_name = 'ja_ginza_electra'

if model_name == 'ja_ginza_electra':
    spacy.prefer_gpu()

nlp = spacy.load(model_name)

num_batch = 100

num_process = max(1, cpu_count() // 4)

def proc(texts):
    docs = list(nlp.pipe(texts, disable=['ner', 'bunsetu_recognizer']))

    for doc in docs:
        for sent in doc.sents:
            print(sent)

    return None

texts = []
for j in tqdm(jsons):
    text = j['text']
    
    kMaxLen = 15000 # NOTE: 49149 bytes = Sudachi's limitation

    if len(text) > kMaxLen:
        continue

    texts.append(text)

    if model_name == 'ja_ginza_electra':
        if len(texts) >= num_batch:
            proc(texts)

            texts = []

# multiprocessing
if model_name != 'ja_ginza_electra':
    total_ticks = max(1, len(texts) // num_batch)
    with tqdm(total=total_ticks) as pbar:
        with concurrent.futures.ProcessPoolExecutor(max_workers=num_process) as executor:
            futures = {executor.submit(proc, texts[i:i+num_batch]): i for i in range(0, len(texts), num_batch)}
      
            results = {}
            for future in concurrent.futures.as_completed(futures):
                arg = futures[future]
                result = future.result()
                pbar.update(1)

