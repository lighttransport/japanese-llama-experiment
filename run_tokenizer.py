from tokenizers import Tokenizer

tokenizer = Tokenizer.from_file("data/tokenizer-cc100-ja.json")

output = tokenizer.encode("吾輩は猫である。名前はまだない。")
print(output.tokens)
print(output.ids)
