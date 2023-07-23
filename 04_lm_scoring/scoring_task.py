import sys
import kenlm
import sentencepiece
import unicodedata
import text_normalizer

from bunkai import Bunkai

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Need ken_lm.arpa sentencepiece.model")
        sys.exit(-1)

    m = kenlm.LanguageModel(sys.argv[1])

    sp = sentencepiece.SentencePieceProcessor()
    sp.load(sys.argv[2])

    bunkai = Bunkai()

    inputs = ['東京はッ晴れ',
        '東京は元気です',
        '吾輩は猫である. 名前はまだない.',
        '東京は晴れ']

    for inp in inputs:
        sentences = bunkai(inp)

        print(sentences)

        for sent in sentences:
            # pretrained model in cc_net uses 'NFD' normalization?
            # TODO: Use https://github.com/facebookresearch/cc_net/blob/main/cc_net/text_normalizer.py
            text = unicodedata.normalize('NFC', sent)
            toks = sp.encode(text, out_type=str)

            tok_input = " ".join(toks)
            ppl = m.perplexity(tok_input)
            print(ppl, sent)

