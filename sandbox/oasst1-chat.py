from datasets import load_dataset
from transformers import AutoTokenizer, AutoModelForCausalLM

tokenizer = AutoTokenizer.from_pretrained("rinna/japanese-gpt-neox-3.6b")

dataset = load_dataset("kunishou/oasst1-chat-44k-ja")

prefix = {
        'human': "### ユーザー: ",
        'gpt': "### アシスタント: "
        }

max_len = 2048

too_large_msgs = 0

for d in dataset['train']:
    cs = d['conversations']
    msgs = [prefix[c['from']] + c['value'] for c in cs]
    msg = "\n".join(msgs)

    token_ids = tokenizer.encode(msg)
    if len(token_ids) > max_len:
        too_large_msgs += 1

print("Conversation too large(> {}): {}".format(max_len, too_large_msgs))
print("Total conversations: {}".format(len(dataset['train'])))

