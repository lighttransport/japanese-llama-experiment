import torch

from transformers import AutoModelForCausalLM, AutoTokenizer
from transformers.generation import GenerationConfig

# Model names: "Qwen/Qwen-7B-Chat", "Qwen/Qwen-14B-Chat"
tokenizer = AutoTokenizer.from_pretrained("llm-jp/llm-jp-13b-instruct-full-jaster-dolly-oasst-v1.0", trust_remote_code=True)

# use bf16
# model = AutoModelForCausalLM.from_pretrained("Qwen/Qwen-7B-Chat", device_map="auto", trust_remote_code=True, bf16=True).eval()
# use fp16
# model = AutoModelForCausalLM.from_pretrained("Qwen/Qwen-7B-Chat", device_map="auto", trust_remote_code=True, fp16=True).eval()
# use cpu only
# model = AutoModelForCausalLM.from_pretrained("Qwen/Qwen-7B-Chat", device_map="cpu", trust_remote_code=True).eval()
# use auto mode, automatically select precision based on the device.
model = AutoModelForCausalLM.from_pretrained(
    "llm-jp/llm-jp-13b-instruct-full-jaster-dolly-oasst-v1.0",
    device_map="auto",
    trust_remote_code=True
    ).eval()


def run(text):
    text = text + "### 回答："
    tokenized_input = tokenizer.encode(text, add_special_tokens=False, return_tensors="pt").to(model.device)
    with torch.no_grad():
            output = model.generate(
                 tokenized_input,
                 max_new_tokens=100,
                 do_sample=True,
                 top_p=0.95,
                 temperature=0.7,
            )[0]
    print(tokenizer.decode(output))

text = "伊勢神宮は何県？"
run(text)

text = """
1. 彼は比類のない陸上選手だ。
2. 彼は比較的に良い陸上選手だ。

1の文が難しいので2の文に直そうと思っているのですが、これってあってますか？
"""
run(text)
