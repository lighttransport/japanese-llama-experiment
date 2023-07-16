import spacy
nlp = spacy.load('ja_ginza_electra')

def do_jp_sentence(text):

    doc = nlp(text)

    res = []

    for sent in doc.sents:

        # https://ja.wikipedia.org/wiki/%E5%93%81%E8%A9%9E

        # - 助詞、助動詞のみは除外(単独で文節を作れない. "ですです" など)
        # - 助詞、助動詞がない文章は除外
        #   - 書き言葉では助詞、助動詞が必要
        # - TODO: 句点はないが, 文章が途中できれているもの(e.g. "東京は晴れてい")

        has_adp = False # 助詞,格助詞
        has_aux = False # 助詞、助動詞, 動詞
        has_noun = False # 名詞とか...
        all_aux = True # すべて助詞、助動詞か

        for token in sent:

            if not token.pos_:
                all_aux = False

            if token.pos_ == 'ADP':
                has_adp = True
            elif token.pos_ == 'AUX':
                has_aux = True
            elif token.pos_ in ('NOUN', 'PRON', 'PROPN'):
                has_noun = True

        if all_aux:
            continue

        if (not has_adp) or (not has_aux):
            #remove
            pass
        else:
            print(sent)

