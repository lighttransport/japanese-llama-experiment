import kenlm
import os

MODEL_BIN='kenlm_model-wiki-nfkc-char.bin'

if __name__ == '__main__':
    if not os.path.exists(MODEL_BIN):
        raise Exception("model file not found: {}".format(MODEL_BIN))
    model = kenlm.LanguageModel(MODEL_BIN)

    for txt in [
        "脱字が存在する文章です。",
        "脱字が存在する文章す。",
        '東京はッ晴れ。',
        '東京は元気です。',
        '吾輩は猫である。 名前はまだない。',
        '吾輩は猫である。 名前はまだな。',
        '東京は晴れ',
        '東京は晴れ。'
    ]:
        sentence = " ".join(txt.strip())
        prob = model.score(sentence, bos=True, eos=True)
        perplexity = model.perplexity(sentence)
        print(perplexity, prob, txt)
