import spacy
nlp = spacy.load('ja_ginza_electra')

text="西田幾多郎は、自分の生い立ちを振り返ってみて、自分の人生の「不幸」は、自分の「我」が「大我」と「超我」の間にある、つまり自分は「小我」に過ぎなかったことに気づかされたのですね。 つまり、「小我」から解放されることによって、生きること、そして「生きる意味」に目覚めることができるというのです。 つまり、人としての「生」における「意味」は、人が「小我」から"

doc = nlp(text)
print(doc)

for sent in doc.sents:
    for token in sent:
        print(token.i)
