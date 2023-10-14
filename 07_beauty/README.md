## beauty pass

最終的なデータセットに整形します.

- ドキュメント json
- lm_score json
- dedup json

を読み込み, duplicate フラグがあるドキュメントは除去し, lm_score で binnning し, 
25600 ドキュメントごとに jsonl.zstd を作ります
1 ファイルは概ね 5 ~ 50 MB 程度になります.

32 chunk(bin) を生成します.

## Step 1

lm_score で全件ソートだと, データセットが大きくなると処理が面倒になるが(オンメモリで処理できないので, out-of-core でソートが必要), 正確なソーティングは不要なため, binning(chunking) で対応します.

lm_score 値のみを収集し, lm_sore 配列を pandas qcut で各 bin が等数になるように 32 分割し, その bin 情報(chunk) をファイルに保存します.

## Step 2

Step1 の bin 情報を用い, それぞれのドキュメントの lm_score の対応する bin を算出し, その bin(chunk) にデータセットを書き出していきます.

## TODO

- より optimal な chunk(bin) 数を求める(32 は適当に選びました)
- 現状は doc/lm_score/dedup json のマージは行数でしか判断していないため, 各ファイルに id を付与してより reliable にする(clean 時点で document に unique id をアサイン)

## Dataset split(optional)

train データセットから validate, test データセット(それぞれ 1 %)を抜き出し, それぞれの dataset を再度 jsonl + zstd 形式で保存します.

huggingface datasets を利用します.


### TODO

* shuffle する?

EoL.
