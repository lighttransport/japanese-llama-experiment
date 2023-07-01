from transformers import AutoTokenizer, AutoModelForCausalLM

tokenizer = AutoTokenizer.from_pretrained("rinna/japanese-gpt-neox-3.6b")

print(tokenizer)

tokenizer.save_pretrained("rinna-3.6b-tokenizer.json")
#print(tokenizer.vocab_file)
