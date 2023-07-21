import sys
import gzip
import json
import glob
import os
import hashlib
from pathlib import Path

import ginza
import zstandard
#import text_normalizer
import url_filtering
import ascii_filtering
import clean_text
import jp_sentence_check

# Disable nlp since its too slow.
#import spacy
#nlp = spacy.load('ja_ginza_electra')

zstd_comp_level = 4 # default = 3

def char_is_hiragana(c):
    return u'\u3040' <= c <= u'\u309F'

def contains_hiragana(s):
    return any(char_is_hiragana(c) for c in s)

def count_whitespaces(text):
    c = 0
    for i in text:
        if i == " ":
            c += 1

    return c


def do_clean(text: str):

    ws_threshold = 1

    # 1. skip text if it does not contain any hiragana.
    if not contains_hiragana(text):
        return None

    # 2. remove sentence if it ends with "...", "," or no punctuation.
    sentences = text.split('\n') # split by '\n'
    results = []
    #print("sentences:", sentences)
    for sent in sentences:
        # 1. remove if the sentence contains some whitespaces
        if count_whitespaces(sent) >= ws_threshold:
            continue
        elif sent.endswith("..."):
            continue
        elif sent.endswith("."):
            # ends with period(after normalization, '。' was replaced to '.')
            pass
        elif sent.endswith(")"):
            # FIXME: May be ascii kaomoji :-)
            pass
        elif sent.endswith("!"):
            pass
        elif sent.endswith("?"):
            pass
        elif sent.endswith(","):
            continue
        elif sent.endswith("\""):
            pass
        elif sent.endswith("'"):
            pass
        else:
            # sentence may be connected to next line.
            pass

        ## limit nlp analysis up to 1024 chars.
        #if len(sent) < 1024:
        #    # resTy = list or None
        #    sent = jp_sentence_check.do_jp_sentence_check(sent, nlp)
        #    #print("jp check:", sent)

        #    if sent is None:
        #        continue

        #    if len(sent) == 0:
        #        continue

        #    sent = "".join(sent)

        results.append(sent)

    if len(results) == 0:
        return None

    return "\\n".join(results)
    
        

def do_filter(line):
    j = json.loads(line)

    ## No url filtering since cc100 ja does not contain URL info
    #if url_filtering.filter_url(j["url"]):
    #    continue

    if ascii_filtering.filter_ascii(j["text"]):
        return None

    # cc_net compatible version. apply NFKC normalization also. 
    j["text"] = do_clean(j["text"])
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
    for i, line in enumerate(lines):
        if (i % 10) == 0:
            print("Processed {} / {}\n".format(i, nlines))
            
        ret = do_filter(line)
        if ret:
            dst_lines.append(json.dumps(ret, ensure_ascii=False))


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
