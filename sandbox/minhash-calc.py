from datasketch import MinHash

from sudachipy import tokenizer 
from sudachipy import dictionary
tokenizer_obj = dictionary.Dictionary().create()

text1 = "東京は元気です"
text2 = "東京は晴れ"
text3 = "吾輩は猫である"

# Use mode B
split_mode = tokenizer.Tokenizer.SplitMode.B
toks1 = [m.surface() for m in tokenizer_obj.tokenize(text1, split_mode)]
toks2 = [m.surface() for m in tokenizer_obj.tokenize(text2, split_mode)]
toks3 = [m.surface() for m in tokenizer_obj.tokenize(text3, split_mode)]

print("data1", toks1)
print("data2", toks2)
print("data3", toks3)

m1, m2, m3 = MinHash(), MinHash(), MinHash()
for d in toks1:
    m1.update(d.encode('utf-8'))
for d in toks2:
    m2.update(d.encode('utf-8'))
for d in toks3:
    m3.update(d.encode('utf-8'))
print("Estimated Jaccard for data1 and data2 is", m1.jaccard(m2))
print("Estimated Jaccard for data1 and data3 is", m1.jaccard(m3))

s1 = set(toks1)
s2 = set(toks2)
s3 = set(toks3)
actual_jaccard1 = float(len(s1.intersection(s2)))/float(len(s1.union(s2)))
actual_jaccard2 = float(len(s1.intersection(s3)))/float(len(s1.union(s3)))
print("Actual Jaccard for data1 and data2 is", actual_jaccard1)
print("Actual Jaccard for data1 and data3 is", actual_jaccard2)
