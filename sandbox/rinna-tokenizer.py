from transformers import AutoTokenizer, AutoModelForCausalLM

tokenizer = AutoTokenizer.from_pretrained("rinna/japanese-gpt-neox-3.6b")

text='''吾輩は猫である。ﾜｶﾞﾊｲ は㈱である.
The primary use of LLaMA is research on large language models, including'''

print(tokenizer.tokenize(text))


#tokenizer.save_pretrained("rinna-3.6b-tokenizer.json")
#print(tokenizer.vocab_file)
