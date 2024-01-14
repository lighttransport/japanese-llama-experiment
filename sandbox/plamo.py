import torch
from transformers import AutoTokenizer, AutoModelForCausalLM
tokenizer = AutoTokenizer.from_pretrained(
    "pfnet/plamo-13b-instruct",
    trust_remote_code=True,
)
model = AutoModelForCausalLM.from_pretrained(
    "pfnet/plamo-13b-instruct",
    trust_remote_code=True,
    torch_dtype=torch.bfloat16,
    device_map="auto",
)

def completion(prompt: str, max_new_tokens: int = 128) -> str:
    inputs = tokenizer(prompt, return_tensors="pt").to("cuda")
    generated_ids = model.generate(
        inputs.input_ids,
        eos_token_id=2,
        pad_token_id=3,
        max_new_tokens=max_new_tokens,
        temperature=1,
        top_p=0.95,
        top_k=50,
        do_sample=True,
    )
    return tokenizer.decode(generated_ids[0], skip_special_tokens=True, clean_up_tokenization_spaces=True)

def generate_prompt(messages: list) -> str:
    sep = "\n\n### "
    prompt = [
        "以下はタスクを説明する指示で、文脈を説明した入力とペアになっています。",
        "要求を適切に補完するよう応答を書いてください。",
    ]
    roles = {"instruction": "指示", "response": "応答", "input": "入力"}
    for msg in messages:
        prompt.append(sep + roles[msg["role"]] + ":\n" + msg["content"])
    prompt.append(sep + roles["response"] + ":\n")
    return "".join(prompt)


prompt = generate_prompt([
    {"role": "instruction", "content": "伊勢神宮は何県？"},
   # {"role": "input", "content": "..."}  ## An extra input (optional)
])
print(completion(prompt, max_new_tokens=128))

text = """
1. 彼は比類のない陸上選手だ。
2. 彼は比較的に良い陸上選手だ。

1の文が難しいので2の文に直そうと思っているのですが、これってあってますか？
"""

prompt = generate_prompt([
    {"role": "instruction", "content": text},
   # {"role": "input", "content": "..."}  ## An extra input (optional)
])
print(completion(prompt, max_new_tokens=128))
