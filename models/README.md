- rwkv_vocab_v20230424.json
  grabbed from https://huggingface.co/RWKV/rwkv-5-world-1b5/tree/aa4409a7ed0720ec6db9d3160237383a87faa20b
  Guess Apache 2.0 license will be applied to vocab file: https://github.com/BlinkDL/ChatRWKV
- rwkv_vocab_v20230424-ja.json
  UTF-8 bytefallback を考慮して 127 を削除. またコードや英単語を主に削り, Emoji と顔文字を追加できる余裕を作ったもの.
- rwkv_vocab_v20230424-ja-emo-kao.json
  rwkv_vocab_v20230424-ja.json に, Emoji と顔文字を追加したもの. '../build_rwkv_world_ja_tokenizer/' で作成.
- tinysegmenter-wiki-51200.model
  - TinySegmenterMaker で wikipedia データセットの一部(分かち書きして shuffle して 1024 * 512 行取り出したもの)で学習したモデルです.
  - MIT ライセンス

- jagger-kwdlc.da

jagger 化した形態素解析辞書

ライセンス(or 利用許諾)は, 京都大学ウェブ文書リード文コーパス に従いします.

https://nlp.ist.i.kyoto-u.ac.jp/?KWDLC

しかし, 京都大学ウェブ文書リード文コーパス には明確なライセンス(or 利用許諾)条項がありません.

https://github.com/ku-nlp/KWDLC/issues/37

利用にあたっては自己責任でお願いします.

