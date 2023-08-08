# 日本語 LLaMa 追加学習のための日本語データセット(日本語コーパス)構築と追加事前学習チャレンジ

Chinese LLaMa を参考に, Japanese LLaMa の追加事前学習のチャレンジをするスクリプト集です.

* 日本語データセット構築(クリーニングと dedup(重複除去))
  * NN G tokens 規模(T.B.D.)
* 日本語データセットで既存の英語ベースの pretain model に対して追加事前学習
  * Chinese LLaMa を参考にしています https://github.com/ymcui/Chinese-LLaMA-Alpaca

の二本立てで構成されています.

## Status

* [x] 日本語データセットの pre cleaning
* [ ] 品質スコアリング計算と dedup
* [ ] 日本語トークナイザ学習
* [ ] 日本語トークナイザで追加事前学習
* [ ] 日本語ファインチューニングデータセットでファインチューニング(Alpaca など)

## 背景

LLM(Large Language Model) のフルの学習(事前学習, pretrain)では, 品質の高いデータセットで学習させることが重要となっています
(as of 2023/07 この傾向ついては今後研究の発展により変わる可能性はあります).

特に文章の品質が高く, また重複や繰り返しが無いデータセットを用意するのが重要になります

* RefinedWeb https://arxiv.org/abs/2306.01116
* SlimPajama https://www.cerebras.net/blog/slimpajama-a-627b-token-cleaned-and-deduplicated-version-of-redpajama

## Requirents

* Python 3.8+
* (mini)conda 環境
* GPU は不要です.

nlp 処理でライブラリのバージョンなどがかち合うため, 2 つ環境を作り, それぞれで

`python -m pip install -r requirements.txt`

`python -m pip install -r requirements-ja-nlp.txt`

で環境構築します.

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
* wiki40b ja

の web から収集された public データセットから, 日本語データセットを構築します.
2 TB ほどストレージを利用します.

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
  * おかしそうな文章の除去など
  * cc100ja のみ sudachipy で形態素解析解析で追加的にクリーニングしています.
    * 処理時間がかかるためほかのデータセットでは使っていない
    * TODO: jaggar なりの高速形態素解析ライブラリを使う.
* [ ] `04_lm_scoring/` KenLM による品質スコアリング
  * KenLM で日本語文章の品質スコアリングを行うメモ https://zenn.dev/syoyo/articles/529ce949121ca4
* [ ] `05_dedup` MinHash での fuzzy dedup および suffix array による exact dedup で重複除去
  * LLM 向け MinHash でテキストの重複除去のメモ https://zenn.dev/syoyo/articles/06eaeb88963b08 

## 日本語トークナイザ作成

T.B.W.

## 追加事前学習

T.B.W.

## TODO

* 追加学習ではなく, SlimPajama と組み合わせ一から事前学習する
  * => あとから日本語知識を付け足すよりは性能がよくなるはず...(学習のコストはかかるが)
 
## License

MIT or Apache 2.0.

japanese-llama-experiment のスクリプトは, ソースコードの SPDX License identifier が明示的に無い場合は, MIT になります.

### Third party licenses

それぞれのファイルを参照ください.
基本的には permissive license(Apache 2.0, MIT など)のみを利用しています.
