import time
import torch
from transformers import AutoTokenizer, AutoModelForCausalLM

model_name = "TheBloke/Xwin-LM-70B-V0.1-GPTQ"
# Note: The default behavior now has injection attack prevention off.
tokenizer = AutoTokenizer.from_pretrained(model_name, trust_remote_code=True)

model = AutoModelForCausalLM.from_pretrained(
    model_name,
    device_map="auto",
    trust_remote_code=True
).eval()
#response, history = model.chat(tokenizer, "你好", history=None)
#print(response)

def prepare_prompt(pred, input_text, output_text, eval_aspect):

    prompt_template = f"""A chat between a curious user and an artificial intelligence assistant.
The assistant gives helpful, detailed, and polite answers to the user's questions.

USER: あなたは採点者です。

問題、採点基準、回答が与えられています。

採点基準を参考にして、回答を1、2、3、4、5の5段階で採点し、採点結果を回答してしてください。

# 問題
{input_text}

# 採点基準
基本的な採点基準:
* 1点: 誤っている、 指示に従えていない
* 2点: 誤っているが、方向性は合っている
* 3点: 部分的に誤っている、 部分的に合っている
* 4点: 合っている
* 5点: 役に立つ

基本的な減点項目:
* 不自然な日本語: -1点
* 部分的に事実と異なる内容を述べている: -1点
* 「倫理的に答えられません」のように過度に安全性を気にしてしまっている: 2点にする

# 問題固有の採点基準
{eval_aspect}

# 回答
{pred}

ASSISTANT: 採点結果
"""

    return prompt_template


input_text = """1. 彼は比類のない陸上選手だ。
2. 彼は比較的に良い陸上選手だ。

1の文が難しいので2の文に直そうと思っているのですが、これってあってますか？
"""

output_text = """いいえ、あまり適切ではありません。 「比類のない」は比べる対象がないほど素晴らしい様を表す言葉です。「比較的に良い」と直してしまうと素晴らしさの強調が弱まってしまうため、以下のように直してみてはいかがでしょうか？

* 彼は飛び抜けた陸上選手だ。
* 彼は唯一無二の陸上選手だ。
* 彼は卓越した陸上選手だ。
"""

eval_aspect = """
出題意図:
* 単に言い換えの正誤判定問題ではなく、ユーザーの意図や困り事を汲んで役に立つAIアシスタントとして振る舞う必要がある

ベースとなる得点:
* 言い換えの正誤判定に正解し、その上でユーザーの役に立つようにそれぞれの単語の意味の説明や、より適切な言い換えを提示する: 5点
* 言い換えの正誤判定に正解しただけ: 4点
* 正誤判定が不正解（言い換えが適切である）: 1点
"""

pred_text = output_text



def run(pred_text, input_text, output_text, eval_aspect):
    print("Pred:", pred_text)
    prompt = prepare_prompt(pred_text, input_text, output_text, eval_aspect)
    #print(prompt)

    inputs = tokenizer(prompt, return_tensors="pt").to(model.device)

    with torch.no_grad():
        s = time.perf_counter() 
        samples = model.generate(**inputs, max_new_tokens=4096, temperature=0.1)
        output = tokenizer.decode(samples[0][inputs["input_ids"].shape[1]:], skip_special_tokens=True)
        e = time.perf_counter()
        print("===>")
        print(output)
        print("<===")

        print("{} secs".format(e - s))


run(pred_text, input_text, output_text, eval_aspect)

pred_text = """はい 2 の文の方が簡単です。比類のない陸上選手とは、非常に優れた陸上選手であることを意味しています。最終的な答え:はい。
"""
run(pred_text, input_text, output_text, eval_aspect)

pred_text = """彼は比較的に良い陸上選手だ。
"""
run(pred_text, input_text, output_text, eval_aspect)
