# wget https://github.com/daac-tools/vaporetto/releases/download/v0.5.0/bccwj-suw+unidic+tag-huge.tar.xz

import vaporetto
import zstandard

dctx = zstandard.ZstdDecompressor()
with open('dict/vaporetto/bccwj-suw+unidic+tag-huge/bccwj-suw+unidic+tag-huge.model.zst', 'rb') as fp:
    with dctx.stream_reader(fp) as dict_reader:
        tokenizer = vaporetto.Vaporetto(dict_reader.read(), predict_tags = True)

toks = tokenizer.tokenize_to_string('外国人参政権')
print(toks)

