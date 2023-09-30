データセットを training で利用可能にする最終パス(beauty pass)です.

* `02_clean_step` の text json
* `04_lm_scoring` の ppl jsonl
* `06_dedup` の dedup flag jsonl

から, 最終的な jsonl.zstd ファイル群を作成します.

dedup flag のあるものは除去し,
Perplexity の低い(品質スコアの高い)順でソートし,
N 行ごとに 1 ファイルにし, M ファイルごとに chunk フォルダを作成します.


## フォルダレイアウト

* train
  * chunk001
  * chunk002
  * `...`
* test
  * chunk001
  * chunk002
  * `...`
* validation
  * chunk001
  * chunk002
  * `...`

## JSON format


* text(string)
* ppl(float) : Perplexity score

```
{ "text": "こんにちは、今日は良い天気ですね。", "ppl": 100.0 }
```

## test

T.B.W.

## validation

T.B.W.
