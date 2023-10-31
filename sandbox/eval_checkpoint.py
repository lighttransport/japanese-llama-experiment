import os
import sys
import torch
from transformers import LlamaForCausalLM, LlamaTokenizer
from transformers.trainer_utils import get_last_checkpoint
from peft import PeftModel, PeftConfig

checkpoint_dir="../10_incremental_pretrain/output_dir/"

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

#for name, module in model.named_modules():
#    print(name)
#
#for name, param in model.named_parameters():
#    print(name)

text = "西田幾多郎は,"
inputs = tokenizer(text, add_special_tokens=False, return_tensors="pt")
print(inputs)

with torch.no_grad():
    output_ids = model.generate(
        **inputs,
        max_new_tokens=100,
        min_new_tokens=100,
        do_sample=True,
        temperature=0.8,
        pad_token_id=tokenizer.pad_token_id,
        bos_token_id=tokenizer.bos_token_id,
        eos_token_id=tokenizer.eos_token_id
    )

output = tokenizer.decode(output_ids.tolist()[0])
print(output)
