import sys
import gzip
import json
import glob
import os
import hashlib
from pathlib import Path

import zstandard
import text_normalizer

#from bunkai import Bunkai

import nltk
from fugashi import Tagger

zstd_comp_level = 5 # default = 3

tagger = Tagger('-Owakati')

# in UTF-8 chars
min_doc_len = 100
max_doc_len = 32700

min_sent_len = 5
max_sent_len = 2048

def hash_text(text):
    return hashlib.md5(text.encode("utf-8")).hexdigest()


def is_repetition_removal(
    text, duplicate_line_fraction=0.3, duplicate_line_character_faction=0.2
):
    """Check if there is repeated content in the input text. Excessive
    repetition is often linked with uninformative content and can be used to
    determine whether it is low-quality text. This function implements
    "Repetition Removal" as described in Gopher_.

    .. _Gopher: https://arxiv.org/abs/2112.11446

    Args:
        text (str): input text.
        duplicate_line_fraction (float, optional): Duplicate row deduplication
            threshold. Defaults to 0.3.
        duplicate_line_character_faction (float, optional): Threshold for the
            proportion of repeated line characters. Defaults to 0.2.

    Returns:
        bool: If there is repeated content in the input text.
    """
    line_count = 0
    dup_line = 0
    dup_line_chars = 0
    visit_lines = {}
    for line in text.split("\n"):
        line_hash = hash_text(line)
        if line_hash in visit_lines:
            dup_line += 1
            dup_line_chars += len(line)
        visit_lines[line_hash] = True

        line_count += 1

    if float(dup_line) / line_count > duplicate_line_fraction:
        return True

    if float(dup_line_chars) / len(text) > duplicate_line_character_faction:
        return True

    top_ngram_character_fractions = [
        (2, 0.2),
        (3, 0.18),
        (4, 0.16),
    ]
    for ngram, threshold in top_ngram_character_fractions:
        #word_list = list(jieba.cut(text))
        # wakachi-gaki
        word_list = tagger.parse(text).split()
        bgs = nltk.ngrams(word_list, ngram)
        fdist = nltk.FreqDist(bgs)
        for word_list, repeat in fdist.items():
            char_count = sum([len(word) for word in word_list])
            if char_count * (repeat - 1) / len(text) > threshold:
                return True

    duplicate_ngram_character_fractions = [
        (5, 0.15),
        (6, 0.14),
        (7, 0.13),
        (8, 0.12),
        (9, 0.11),
        (10, 0.10),
    ]
    for ngram, threshold in duplicate_ngram_character_fractions:
        fdist = {}
        word_list = tagger.parse(text).split()
        mark = [0] * len(word_list)
        for i in range(len(word_list) - ngram + 1):
            bag = tuple(word_list[i: i + ngram])
            if bag in fdist:
                for j in range(i, i + ngram):
                    mark[j] = len(word_list[j])
                fdist[bag] += 1
            else:
                fdist[bag] = 1

        if sum(mark) / float(len(text)) > threshold:
            return True

    return False

def do_length_filter(text: str):

    if len(text) < min_doc_len:
        return None

    if len(text) > max_doc_len:
        return None

    out_texts = []

    sents = text.split('\n')

    for sent in sents:
        if len(sent) < min_sent_len:
            return None

        if len(sent) > max_sent_len:
            return None

        out_texts.append(sent)

    return "\n".join(out_texts)
        
def do_repetition_removal(text: str):

    # Use normalizer for dedup(e.g. replace the number with a placeholder(0))
    in_text = text_normalizer.normalize_for_dedup(text)
    if is_repetition_removal(in_text):
        return None

    return text

def do_filter(line):
    j = json.loads(line)

    j["text"] = do_length_filter(j["text"])
    if j["text"] is None:
        return None

    j["text"] = do_repetition_removal(j["text"])
    if j["text"] is None:
        return None

    return j

def worker(in_filepath, out_filepath):


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
    nfiltered = 0
    for i, line in enumerate(lines):
        if (i % 100) == 0:
            print("Processed {} / {} (completely filtered {})\n".format(i, nlines, nfiltered))
            
        ret = do_filter(line)
        if ret:
            dst_lines.append(json.dumps(ret, ensure_ascii=False))
        else:
            nfiltered += 1


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
   
    return "  {} => {}".format(nlines, len(dst_lines))

if __name__ == '__main__':
    assert len(sys.argv) > 2

    in_filepath = sys.argv[1]
    out_filepath = sys.argv[2]

    ret = worker(in_filepath, out_filepath)
    print(ret)
