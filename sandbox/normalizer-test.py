from tokenizers import Tokenizer
from tokenizers import normalizers
from tokenizers.normalizers import NFKC

#tokenizer = Tokenizer.from_file("../tokenizer-cc100-ja.json")

text = "ﾜｶﾞﾊｲは㈱である. 吾輩は猫である。名前はまだない。"

normalizer = normalizers.Sequence([NFKC()])
print(normalizer.normalize_str(text))

#tokenizer.normalizer = normalizers.sequ
#output = tokenizer.encode("ﾜｶﾞﾊｲは㈱である. 吾輩は猫である。名前はまだない。")
#print(output.tokens)
#print(output.ids)
