#import spacy
#nlp = spacy.load('ja_ginza_electra')

from bunkai import Bunkai

from sudachipy import tokenizer
from sudachipy import dictionary


def do_jp_text_check(text, tokenizer_obj):

    tokens = tokenizer_obj.tokenize(text)

    res = None

    # https://ja.wikipedia.org/wiki/%E5%93%81%E8%A9%9E

    # - 助詞、助動詞のみは除外(単独で文節を作れない. "ですです" など)
    # - 助詞、助動詞がない文章は除外
    #   - 書き言葉では助詞、助動詞が必要
    # - TODO: 句点はないが, 文章が途中できれているもの(e.g. "東京は晴れてい")
    #   - 終わりが終止形でない場合? ('動詞', '非自立可能', '*', '*', '上一段-ア行', '連用形-一般')

    has_adp = False # 助詞,格助詞
    has_aux = False # 助詞、助動詞, 動詞
    has_noun = False # 名詞とか...
    all_aux = True # すべて助詞、助動詞か

    for token in tokens:

        pos = token.part_of_speech()
        print(pos)
        if pos[0] == '助詞':
            pass
        else:
            all_aux = False

        if pos[1] == '格助詞':
            has_adp = True
        elif pos[0] in ('助詞', '助動詞', '動詞'):
            has_aux = True
        elif pos[0] == '名詞':
            has_noun = True

    if all_aux:
        return None

    if (not has_adp) and (not has_aux):
        return None

    return text

if __name__ == '__main__':
    tokenizer_obj = dictionary.Dictionary().create()

    text = '東京は晴れ. 吾輩は猫である. 名前はまだない? 東京第一. 東京は晴れてい'

    bunkai = Bunkai()

    sentences = bunkai(text)

    for sent in sentences:
        ret = do_jp_text_check(sent, tokenizer_obj)
        print(ret)
