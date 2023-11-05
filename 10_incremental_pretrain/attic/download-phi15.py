import torch
from transformers import AutoModelForCausalLM, AutoTokenizer

torch.set_default_device("cpu")
model = AutoModelForCausalLM.from_pretrained("microsoft/phi-1_5", trust_remote_code=True)
tokenizer = AutoTokenizer.from_pretrained("microsoft/phi-1_5", trust_remote_code=True)

