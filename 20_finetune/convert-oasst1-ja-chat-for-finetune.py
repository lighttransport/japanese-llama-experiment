# - prompt の置き換え
# - メッセージ長チェック

from transformers import AutoTokenizer, AutoModelForCausalLM
import os
import json

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
    outj.append(json.dumps(m))

outbuf = "\n".join(outj)

with open(outfname, "w") as f:
    f.write(outbuf)
