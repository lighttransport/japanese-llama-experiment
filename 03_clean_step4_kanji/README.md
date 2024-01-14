## 目的

日本語で使われる漢字のみ(おもには常用漢字と人名用漢字)で文章が構成されるように fintering する.
(旧字体などでは embedding が変わってしまうため)

## Requirements

go 1.6 or later
(otherwise `go install` fails)

## Install

```
$ go install github.com/ikawaha/kanji
go: downloading github.com/ikawaha/kanji v1.1.1
package github.com/ikawaha/kanji is not a main package

# You can ignore `package github.com/ikawaha/kanji is not a main package` message.
```

## Steps

本ステップでは以下の処理を行います.

- 1. 中国語固有の漢字がある場合の除去
  - 繁体字と簡体字と日本語を区別する https://qiita.com/Saqoosha/items/927e9d6e77922ad9f08a
  - 文字列が中国語（簡体字）かどうか雑に判定する https://qiita.com/ry_2718/items/47c21792d7bbd3fe33b9
- 2. 異体字正規化 https://github.com/ekurerice/ja_cvu_normalizer
- 3. 旧字体を新字体へ変換 https://github.com/ikawaha/kanji/
  - golang プロセスを subprocess 起動して処理

## Note

ある程度は中国語漢字, 異体字/旧字体などがあったほうがよいような気もする...
(filtering するかどうか, 確率的にやってもよいかもしれません)

EoL.
