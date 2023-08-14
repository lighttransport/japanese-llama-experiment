import sys
import kenlm
import sentencepiece
import unicodedata
import text_normalizer
import zstandard

from bunkai import Bunkai

if __name__ == '__main__':
    if len(sys.argv) < 6:
        print("Need ken_lm.arpa sentencepiece.model in_dataset.jsonl.zstd out_datasetname text_keyname_in_json")
        sys.exit(-1)

    m = kenlm.LanguageModel(sys.argv[1])

    sp = sentencepiece.SentencePieceProcessor()
    sp.load(sys.argv[2])

    in_filename = sys.argv[3]
    out_filename = sys.argv[4]
    text_key = sys.argv[5]

    bunkai = Bunkai()

    inputs = ['東京はッ晴れ',
        '東京は元気です',
        '吾輩は猫である. 名前はまだない.',
        '東京は晴れ']

    for inp in inputs:
        sentences = bunkai(inp)

        print(sentences)

        for sent in sentences:
            # pretrained model in cc_net seems using'NFD' normalization
            # Use https://github.com/facebookresearch/cc_net/blob/main/cc_net/text_normalizer.py
            # For Japanese text, usually 'NFKC' is used.
            # This may cause some discrepancy in tokenization.

            text = unicodedata.normalize('NFKC', sent)
            toks = sp.encode(text, out_type=str)

            tok_input = " ".join(toks)
            ppl = m.perplexity(tok_input)
            print(ppl, sent)

