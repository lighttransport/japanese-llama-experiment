import os
import sys
import json
import kenlm
import sentencepiece
import unicodedata
import text_normalizer
import zstandard

#from bunkai import Bunkai

zstd_comp_level = 5

def pp(log_score, length):
    return 10.0 ** (-log_score / length)


def doc_perplexity(model, lines, normalize: bool = False):
    """
    lines: tokenized string(delimiter: whitespace)
    """

    doc_log_score, doc_length = 0, 0
    for line in lines:
        if normalize:
            line = text_normalizer.normalize(line)
        log_score = model.score(line)
        length = len(line.split()) + 1
        doc_log_score += log_score
        doc_length += length

    return round(pp(doc_log_score, doc_length), 1)


def worker(in_filepath, out_filepath, model, spm, text_key):


    print(in_filepath)

    with open(in_filepath, 'rb') as f:
        indata = f.read()

    basefilename = os.path.basename(in_filepath)

    dctx = zstandard.ZstdDecompressor()
    dobj = dctx.decompressobj()
    jsonldata = dobj.decompress(indata)

    lines = jsonldata.splitlines()
    del indata

    dst_lines = []

    nlines = len(lines)

    results = []
    for i, line in enumerate(lines):
        if (i % 100) == 0:
            print("Processed {} / {}\n".format(i, nlines))
            
        j = json.loads(line)

        in_toks = []
        for in_text in j[text_key].split():
            if len(in_text) < 3:
                continue

            toks = sp.encode(in_text, out_type=str)
            in_toks.append(" ".join(toks))

        assert len(in_toks) > 0

        # Assume sentence is splitted by newline 
        # Assume normalize is already done in 01_normalize step.
        ppl = doc_perplexity(model, in_toks, normalize=False)

        out_j = {}
        out_j["lm_score"] = ppl

        dst_lines.append(json.dumps(out_j, ensure_ascii=False))

    del lines

    zctx = zstandard.ZstdCompressor(level=zstd_comp_level)
    zfilename = out_filepath

    if len(dst_lines) == 0:
        return "Invalid"

    dst_buf = "\n".join(dst_lines)

    # TODO: Use stream
    zcompressed = zctx.compress(bytes(dst_buf, 'utf-8'))

    print("write to ", zfilename)
    of = open(zfilename, 'wb')
    of.write(zcompressed)
    of.close()


    del dst_buf
    del zcompressed



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

    worker(in_filename, out_filename, m, sp, text_key)

    #bunkai = Bunkai()

    #inputs = ['東京はッ晴れ',
    #    '東京は元気です',
    #    '吾輩は猫である. 名前はまだない.',
    #    '東京は晴れ']

    #for inp in inputs:
    #    sentences = bunkai(inp)

    #    print(sentences)

    #    lines = []

    #    for sent in sentences:
    #        # pretrained model in cc_net seems using'NFD' normalization
    #        # Use https://github.com/facebookresearch/cc_net/blob/main/cc_net/text_normalizer.py
    #        # For Japanese text, usually 'NFKC' is used.
    #        # This may cause some discrepancy in tokenization.

    #        text = unicodedata.normalize('NFKC', sent)
    #        toks = sp.encode(text, out_type=str)

    #        tok_input = " ".join(toks)
    #        lines.append(tok_input)

    #    ppl = doc_perplexity(m, lines, normalize=False)
    #    print(ppl, lines)
