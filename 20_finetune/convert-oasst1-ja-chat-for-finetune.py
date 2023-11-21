# - 句点の変換
# - prompt の置き換え
# - メッセージ長チェック

import re
import unicodedata

from transformers import AutoTokenizer, AutoModelForCausalLM
import os
import json

UNICODE_PUNCT = {
    # FIXME: Japanese TinyLlama specific. 他の日本語 LLM model では変換不要
    "，": ",",
    "。": ".",
    "、": ",",
    }

UNICODE_PUNCT_RE = re.compile(f"[{''.join(UNICODE_PUNCT.keys())}]")


def replace_unicode_punct(text: str) -> str:
    return "".join((UNICODE_PUNCT.get(c, c) for c in text))

# -4 for safety
max_len = 2048 - 4

jp_tinyllama_tokenizer_path = "../10_incremental_pretrain/Japanese-TinyLlama-1.1B-1.0T/"
tokenizer = AutoTokenizer.from_pretrained(jp_tinyllama_tokenizer_path)

j = json.loads(open("oasst1-ja-chat-multiturn.json").read())

role_prefix = {
    'prompter': "### ユーザー: ",
    'assistant': "### アシスタント: "
    }

conversations = []
print("Input len: ", len(j))
for d in j:
    msgs = [role_prefix[c['role']] + c['text_ja'] for c in d]

    msg = "\n".join(msgs)

    msg = replace_unicode_punct(msg)

    input_ids = tokenizer.encode(msg)

    # length check
    if len(input_ids) > max_len:
        continue
        
    conversations.append(msg)

print("after max_len filtered: ", len(conversations))

# to jsonl format
outfname = "oasst1-ja-chat-multiturn-sft.jsonl"
outj = []
for item in conversations:
    m = {'text_ja': item}
    outj.append(json.dumps(m, ensure_ascii=False))

outbuf = "\n".join(outj)

with open(outfname, "w") as f:
    f.write(outbuf)
