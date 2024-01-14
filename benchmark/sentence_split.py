import sys

# import ../common_util/ modules
sys.path.append("..")
from common_util import load_jsonl_zstd

jsons = load_jsonl_zstd.load("../data/00_dataset/cc100/cc100-ja.00000.jsonl.zstd")

import tqdm
import spacy
spacy.prefer_gpu()
nlp = spacy.load('ja_ginza_electra')

num_batch = 100

texts = []
for j in tqdm.tqdm(jsons):
    text = j['text']
    #print(text)
    
    kMaxLen = 15000 # NOTE: 49149 bytes = Sudachi's limitation

    if len(text) > kMaxLen:
        continue

    texts.append(text)

    if len(texts) >= num_batch:

        docs = list(nlp.pipe(texts, disable=['ner', 'bunsetu_recognizer']))
        for doc in docs:
            for sent in doc.sents:
                print(sent)
        
        texts = []

