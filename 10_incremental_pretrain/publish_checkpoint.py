import os
import sys
import torch
from transformers import LlamaForCausalLM, LlamaTokenizer
from transformers.trainer_utils import get_last_checkpoint
from peft import PeftModel, PeftConfig

pretrain_output_dir="Japanese-TinyLlama-1.1B-1.0T"

checkpoint_dir="output_dir/"

last_checkpoint = get_last_checkpoint(checkpoint_dir)
if last_checkpoint is None and len(os.listdir(checkpoint_dir)) > 0:
    raise ValueError(
        f"Output directory ({checkpoint_dir}) already exists and is not empty. "
        "Use --overwrite_output_dir to overcome."
    )

print(last_checkpoint)

tokenizer = LlamaTokenizer.from_pretrained(last_checkpoint)
print(tokenizer)

peft_model_path = last_checkpoint

config = PeftConfig.from_pretrained(peft_model_path)
print(config.base_model_name_or_path)
model = LlamaForCausalLM.from_pretrained(config.base_model_name_or_path, device_map="cpu")

# To reconstruct Japanese TinyLLaMa model,
# 1. load base TinyLlama model
# 2. resize embeddings
# 3. load LoRA weights
model_vocab_size = model.get_output_embeddings().weight.size(0)
if (model_vocab_size != len(tokenizer)):
    model.resize_token_embeddings(len(tokenizer), pad_to_multiple_of=64)

print(model)

model = PeftModel.from_pretrained(model, peft_model_path)
print("peft", model)


# merge
merged_model = model.merge_and_unload()

# save
os.makedirs(pretrain_output_dir, exist_ok=True)
merged_model.save_pretrained(pretrain_output_dir)
tokenizer.save_pretrained(pretrain_output_dir)

print("Wrote merged pretrained model to: ", pretrain_output_dir)

# eval test

text = "ずんだもんは、 東北に住むかわいい妖精です。"
inputs = tokenizer(text, add_special_tokens=False, return_tensors="pt")
print(inputs)

with torch.no_grad():
    output_ids = merged_model.generate(
        **inputs,
        max_new_tokens=2048,
        min_new_tokens=250,
        do_sample=True,
        num_beams=1,
        temperature=0.8,
        no_repeat_ngram_size=2,
        pad_token_id=tokenizer.pad_token_id,
        bos_token_id=tokenizer.bos_token_id,
        eos_token_id=tokenizer.eos_token_id
    )
