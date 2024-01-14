from transformers import AutoModelForCausalLM, AutoTokenizer
from transformers.generation import GenerationConfig

# Model names："Qwen/Qwen-7B-Chat"、"Qwen/Qwen-14B-Chat"
tokenizer = AutoTokenizer.from_pretrained("Qwen/Qwen-14B-Chat", trust_remote_code=True)
model = AutoModelForCausalLM.from_pretrained("Qwen/Qwen-14B-Chat", device_map="cpu", trust_remote_code=True).eval()

response, history = model.chat(tokenizer, "Translate into Japanese: Hello world.", history=None)
print(response)
