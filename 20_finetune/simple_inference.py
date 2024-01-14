from transformers.trainer_utils import get_last_checkpoint

from transformers import AutoTokenizer
import transformers 
import torch
model_path = get_last_checkpoint("./output/jp-tinyllama-1T_FT_lr1e-5_ep5")
print(model_path)

tokenizer = AutoTokenizer.from_pretrained(model_path)
pipeline = transformers.pipeline(
    "text-generation",
    model=model_path,
    torch_dtype=torch.float16,
    device_map="auto",
)

prompt = "なぜチョコレートは甘いの?"
formatted_prompt = (
    f"### ユーザー: {prompt} ### アシスタント:"
)


sequences = pipeline(
    formatted_prompt,
    do_sample=True,
    top_k=50,
    top_p = 0.9,
    num_return_sequences=1,
    repetition_penalty=1.1,
    max_new_tokens=1024,
)
for seq in sequences:
    print(f"Result: {seq['generated_text']}")
