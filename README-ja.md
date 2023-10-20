# 日本語 LLaMa 追加学習のための日本語データセット(日本語コーパス)構築と追加事前学習チャレンジ

Chinese LLaMa を参考に, Japanese LLaMa の追加事前学習のチャレンジをするスクリプト集です.

* 日本語データセット構築(クリーニングと dedup(重複除去))
  * 59 B tokens 規模(NSFW フィルタなし)
  * フィルタリング後 dataset https://huggingface.co/datasets/lighttransport/Japanese-CharShu-59B
    * 現状は private. リーガルチェック後公開予定

[charshu](imgs/charshu.png)

* 日本語データセットで既存の英語ベースの pretain model に対して追加事前学習
  * Chinese LLaMa を参考にしています https://github.com/ymcui/Chinese-LLaMA-Alpaca

の二本立てで構成されています.

## Status

* [x] データセットの取得 [00_download_dataset](00_download_dataset)
* [x] データセットの前処理 [01_prepare_dataset](01_prepare_dataset)
  * jsonl + zstd 形式への変換
* [x] テキストの正規化 [02_normalize](02_normalize)
  * NFKC で正規化
  * 句読点は現在「, .」. 「、。」にしたほうがいいか?
* [x] 日本語データセットの pre cleaning [03_clean_step1](03_clean_step1/)
* [x] bunkai による改行を考慮した文分解.
* [ ] NG ワードなどでの filtering.
  * [ ] HojiChar 利用予定
* [x] 品質スコアリング計算 [04_lm_scoring](04_lm_scoring)
  * [x] KenLM の Perplexity で品質を計算
* [x] dedup(重複除去) [05_dedup](05_dedup)
  * [x] MinHash fuzzy dedup
  * [ ] (optional) suffix array exact dedup
* [x] 最終的なデータセット形態(Beauty shot)の作成 [07_beauty](07_beauty)
  * 品質スコアでソート(bucketize)
* [x] 日本語トークナイザ学習
  * [train_tokenizer](train_tokenizer/)
  * LLaMa tokenizer へのマージ
* [ ] 日本語トークナイザとクリーニングした日本語データセットで追加事前学習(incremental pre-training)
* [ ] 日本語ファインチューニングデータセットでファインチューニング(Alpaca など)

## 背景

LLM(Large Language Model) のフルの学習(事前学習, pretrain)では, 品質の高いデータセットで学習させることが重要となっています
(as of 2023/07 この傾向ついては今後研究の発展により変わる可能性はあります).

特に文章の品質が高く, また重複や繰り返しが無いデータセットを用意するのが重要になります

* RefinedWeb https://arxiv.org/abs/2306.01116
* SlimPajama https://www.cerebras.net/blog/slimpajama-a-627b-token-cleaned-and-deduplicated-version-of-redpajama

## Requirents

* cmake + C++ 14 compiler
  * clang 推奨
* Python 3.8+
* (mini)conda 環境
* 128 GB CPU mem PC.
* GPU は不要です.

nlp 処理でライブラリのバージョンなどがかち合うため, 2 つ環境を作り, それぞれで

`python -m pip install -r requirements.txt`

`python -m pip install -r requirements-ja-nlp.txt`

で環境構築します.

## Build CPP module

dedup など処理の効率化のために C++ で処理を行います.
(cmake は pip で入るのを利用するとよいでしょう)

```
$ cd cpp
$ ./bootstrap.sh
$ cd build
$ make
```

## Setup


https://github.com/lighttransport/data-for-japanese-corpus

にデータセット構築用の各種学習済みモデル(e.g. KenML)があります.

```
$ git clone https://github.com/lighttransport/data-for-japanese-corpus
```

## クリーニング済み日本語データセット構築

huggingface datasets に, 下記スクリプトで処理した各ステージのデータセットを用意しています.

https://huggingface.co/datasets/lighttransport/japanese-dataset-cleaned-experiment

(現在 public 公開にするためにライセンス関連リーガルチェック中)

本 repo のスクリプトでは,

* cc100ja
* mc4 ja
* OSCAR 2301 ja
* TODO: wiki40b ja

の web から収集された public データセットから, 日本語データセットを構築します.
概ね各処理ステップごとにデータを保存するため, 1 TB ほどストレージを利用します.

データセットはすべて jsonl 形式 + zstd 圧縮にしています.
dedup までは各データセットごとに jsonl + zstd ファイルを用意し,
dedup 後にひとつの jsonl + zstd のセットにまとめます.

### 処理ステップ

* [x] `00_download_dataset/` データセットのダウンロード
* [x] `01_prepare_dataset/` データセットの前処理(jsonl + zstd 形式への変換)
* [x] `02_normalize/` テキストの正規化
  * 正規化はトークナイズに行うこともできますが, 正規化機能を持たないトークナイザ(llama.cpp など)の対応のために最初のほうで正規化します.
* [x] `03_clean_step1/` 簡単な pre cleaning
  * URL でフィルタリング
  * bunkai での文境界判定
  * おかしそうな文章の除去など
  * cc100ja のみ sudachipy で形態素解析解析で追加的にクリーニングしています.
    * 処理時間がかかるためほかのデータセットでは使っていない
    * TODO: jaggar なりの高速形態素解析ライブラリを使う.
  * 1 CPU でマルチプロセス処理で概ね 1 ~ 2 日かかります.
* [x] `03_clean_step2/`
  * 文章の長さでフィルタ
  * repetition removal. 文章中に繰り返しがあるかの判定
  * 1 CPU で 4~5 時間程度
* [x] `04_lm_scoring/` KenLM による品質スコアリング
  * KenLM で日本語文章の品質スコアリングを行うメモ https://zenn.dev/syoyo/articles/529ce949121ca4
  * 1 CPU で 20 分程度
* [x] `05_minhash` MinHash での hash 計算
  * LLM 向け MinHash でテキストの重複除去のメモ https://zenn.dev/syoyo/articles/06eaeb88963b08
  * 16 cores CPU(e.g. Ryzen 3950X) x 1 で概ね 4 時間(57 B tokens データに対して)
* [x] `06_dedup` 05_minhash で求まった hash で dedup
  * 1 CPU 1 core で概ね 50 分(57 B tokens データに対して)
* [x] `07_beauty`
  *  training に回せる形に jsonl ファイル群を整理
  * lm_score で bucketize(chunk が若いほど品質スコアの高いドキュメント)

## トークン量(UTF-8 文字数)

* dedup 後に 59 B chars
  * NSFW フィルタなどは未適用
  * tokenier 次第ですが, 40 B ~ 50 B tokens 規模になるでしょう.

## 正規化

NFKC 形式(sentencepiece の normalizer のデフォルト?)で正規化します.
NFKC は日本語の正規化でよく使われる形式のようです.
たとえば Rinna はトークナイザを見る限りでは NFKC で正規化をしています.

しかし, LLaMa1 では NFD が利用されているようです.
(cc_net の text_normalizer)

NFD ですと, 日本語では「が」が「か　”」などと濁点が分解されてしまい, 日本語との相性がよくないと思われます.

ただ, 日本語データセットには英語はあまり含まれていないため, NFC 正規化した英語データセットでの事前学習にたいして, NFKC 正規化で日本語(英語少しふくむ)データセットで追加事前学習してもトークン表現などが異なり学習の精度が落ちることは無いと思います.
(=> 本来は, 日本語を NFC で正規化したのでどうなるか比較するのがよいが...)

幸いにも, SlimPajama などの LLaMa の open 再現では NFC を使っています(たとえば, 濁点は分解されない).
(llama2 はどうなっているか不明)

したがって, 少なくとも SlimPajama データセットで学習したモデルにたいして, NFKC で正規化した日本語データセットで追加事前学習するのは大丈夫と言えそうです.

## 文境界判定による文の分解

web からのデータでは文途中に改行が入っていたりします.

bunkai で文を適切に抽出します.

https://github.com/megagonlabs/bunkai

改行対応版(機械学習利用. 処理時間はかかる)を利用します.

```
$ pip install -U 'bunkai[lb]'
$ bunkai --model bunkai-model-directory --setup
```

改行文字は ▁ (U+2581) を利用します.

```py

from pathlib import Path

from bunkai import Bunkai

bunkai = Bunkai(path_model=Path("bunkai-model-directory"))
for sentence in bunkai("そうなんです▁このように▁pythonライブラリとしても▁使えます！"):
    print(sentence)

```


TODO: cc100ja などによく出現する, 三点リーダー(`...`)を扱えるようにする.

## 繰り返しの除去

TODO

https://github.com/shjwudp/c4-dataset-script

のスクリプトを参考にして除去します.

## 品質スコアリング


Wikipedia 文章を NFKC 正規化して KenLM で学習し, それを用いて品質スコアリング(Perplexity 算出)します.

pretrain したモデルは以下から取得できます.

https://huggingface.co/lighttransport/japanese-scoring-model

分かち書き版は 9.5 GB くらいあるので注意ください.

(ccnet にも pretrain された KenLM model がありますが, ccnet のほうは NFD 正規化しているような気がしますので, 入力を NFD 正規化しないとうまく Perplexity が算出できないかもしれません)


## dedup

### Minhash で fuzzy dedup

minhash を求め, それを元に Fuzzy dedup を行います.

minhash は 5-gram, 20 x 10 buckets(The Pile と同じ構成)でハッシュを 200 個計算します.

TODO:

- [ ] RefinedWeb にしたがって 20 x 450 の 9000 ハッシュを計算するようにする?
- [ ] minhash で false positive 対策のため, Jaccard 係数求める additional step も実装する
  - ただ, データセットサイズが大きいと Jaccard 係数求めて dedup は難しいところであるため, RefinedWeb では Jaccard 係数算出は行っていない

### optional: Suffix array で exact dedup

TODO.

## beauty

[07_beauty/README.md](07_beauty/README.md) を参照ください.

## 日本語トークナイザ作成

See [train_tokenizer/](train_tokenizer/)

Unigram で学習し, その後 LLaMa tokenizer に merge します.

## 追加事前学習

TODO

[10_incremental_pretrain/](10_incremental_pretrain/)

## Known Issue

mc4 データセットを 03_clean_step1 or 03_clean_step2 で処理すると json データが壊れる(Invalid な Unicode 文字データ)ことがあります.
(python マルチプロセス + bunkai(pytorch) あたりでなにかデータレースが発生?)

## TODO

* 追加学習ではなく, SlimPajama と組み合わせ一から事前学習する
  * => あとから日本語知識を付け足すよりは性能がよくなるはず...(学習のコストはかかるが)
* 照応解析や構文解析を行い, よりよい日本語文章の判定を行う
  * コンテキストが違う文がミックスされているのをはじくなど

## License

MIT or Apache 2.0.

japanese-llama-experiment のスクリプトは, ソースコードの SPDX License identifier が明示的に無い場合は, MIT になります.

### Third party licenses

それぞれのファイルを参照ください.
基本的には permissive license(Apache 2.0, MIT など)のみを利用しています.
