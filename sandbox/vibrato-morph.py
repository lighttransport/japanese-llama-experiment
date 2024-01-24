#
# Download dict: https://github.com/daac-tools/vibrato/releases/download/v0.5.0/bccwj-suw+unidic-cwj-3_1_1.tar.xz
#
import vibrato
import zstandard

dctx = zstandard.ZstdDecompressor()
with open('dict/vibrato/bccwj-suw+unidic-cwj-3_1_1/system.dic.zst', 'rb') as fp:
    with dctx.stream_reader(fp) as dict_reader:
        tokenizer = vibrato.Vibrato(dict_reader.read())

toks = tokenizer.tokenize('外国人参政権')
for t in toks:
    print(t)
