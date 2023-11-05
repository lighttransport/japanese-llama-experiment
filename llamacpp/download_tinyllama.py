from transformers import AutoTokenizer, AutoModelForCausalLM
import transformers
import torch

model_name = "PY007/TinyLlama-1.1B-intermediate-step-715k-1.5T"
tokenizer = AutoTokenizer.from_pretrained(model_name)
model = AutoModelForCausalLM.from_pretrained(model_name)
