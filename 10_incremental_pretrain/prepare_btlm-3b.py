from transformers import AutoTokenizer, AutoModelForCausalLM

# Load&save the tokenizer and model
tokenizer = AutoTokenizer.from_pretrained("cerebras/btlm-3b-8k-base")
model = AutoModelForCausalLM.from_pretrained("cerebras/btlm-3b-8k-base", trust_remote_code=True, torch_dtype="auto")
