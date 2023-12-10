# Download oasst1 file: https://huggingface.co/datasets/OpenAssistant/oasst1/resolve/main/2023-04-12_oasst_ready.messages.jsonl.gz
import os
import sys
import gc

import torch
import json
import tqdm
from transformers import AutoModelForCausalLM, AutoTokenizer
from transformers import GenerationConfig
from qwen_generation_utils import make_context, decode_tokens, get_stop_words_ids

out_dir = "output"
os.makedirs(out_dir, exist_ok=True)

# ignore message which exceeds `max_chars` to avoid oom error. 
# TODO: limit the number of ids after tokenization
max_chars = 1500

# up to 2 for 3090(24GB)
nbatch = 2

# to resume processing. -1 = start from the begining.
skip_count = 106

# load oasst1 dataset

msgs = []
lines = open("2023-04-12_oasst_ready.messages.jsonl", "r").readlines()
for l in lines:
    msgs.append(json.loads(l))


# NOTE 14B + Int8 fits well 3090(24GB)
#model_path = "qwen/Qwen-14B-Chat-Int8"

model_path = "qwen/Qwen-14B-Chat"
tokenizer = AutoTokenizer.from_pretrained(
    model_path,
    pad_token='<|extra_0|>',
    eos_token='<|endoftext|>',
    padding_side='left',
    trust_remote_code=True
)
model = AutoModelForCausalLM.from_pretrained(
    model_path,
    pad_token_id=tokenizer.pad_token_id,
    device_map="auto",
    use_cache_quantization=True,
    use_cache_kernel=True,
    torch_dtype=torch.bfloat16,
    trust_remote_code=True
).eval()
model.generation_config = GenerationConfig.from_pretrained(model_path, pad_token_id=tokenizer.pad_token_id)


batch_count = 0
for i in tqdm.tqdm(range(0, len(msgs), nbatch)):
    if skip_count > 0:
        if i < skip_count:
            continue

    batch_msgs = msgs[i:i+nbatch]

    all_raw_text = []
    for m in batch_msgs:
        if len(m['text']) > max_chars:
            continue

        all_raw_text.append("Translate into Japanse: " + m['text'])

    if len(all_raw_text) == 0:
        continue

    batch_raw_text = []
    for q in all_raw_text:
        raw_text, _ = make_context(
            tokenizer,
            q,
            system="You are a helpfull assistant.",
            max_window_size=model.generation_config.max_window_size,
            chat_format=model.generation_config.chat_format,
        )
        batch_raw_text.append(raw_text)

    with torch.no_grad():
        batch_input_ids = tokenizer(batch_raw_text, padding='longest')
        batch_input_ids = torch.LongTensor(batch_input_ids['input_ids']).to(model.device)
        batch_out_ids = model.generate(
            batch_input_ids,
            return_dict_in_generate=False,
            generation_config=model.generation_config
        )

        padding_lens = [batch_input_ids[i].eq(tokenizer.pad_token_id).sum().item() for i in range(batch_input_ids.size(0))]

        batch_response = [
            decode_tokens(
                batch_out_ids[i][padding_lens[i]:],
                tokenizer,
                raw_text_len=len(batch_raw_text[i]),
                context_length=(batch_input_ids[i].size(0)-padding_lens[i]),
                chat_format="chatml",
                verbose=False,
                errors='replace'
            ) for i in range(len(all_raw_text))
        ]

    for i, r in enumerate(batch_response):
        print(r)
        batch_msgs[i]['text_ja'] = r


    # 1000 files per directory
    out_basesubdir = str(batch_count // 1000)
    os.makedirs(os.path.join(out_dir, out_basesubdir), exist_ok=True)

    out_filename = "oasst1-ja-{:05d}.jsonl".format(batch_count)

    out_filepath = os.path.join(out_dir, os.path.join(out_basesubdir, out_filename))
    with open(out_filepath, "w") as f:
        jsonl = [] 
        for b in batch_msgs:
            jsonl.append(json.dumps(b, ensure_ascii=False))

        jsonl = "\n".join(jsonl)
        f.write(jsonl)

    batch_count += 1

    del all_raw_text
    del batch_raw_text

    del batch_input_ids
    del batch_out_ids

    gc.collect()
    if torch.cuda.is_available():
        torch.cuda.empty_cache()

