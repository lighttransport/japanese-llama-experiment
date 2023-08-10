from fugashi import Tagger

tagger = Tagger('-Owakati')
text = "麩菓子は、麩を主材料とした日本の菓子。"
ret = tagger.parse(text)
print(ret)
