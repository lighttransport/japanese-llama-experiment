## beauty pass

最終的なデータセットに整形します.

- ドキュメント json
- lm_score json
- dedup json

を読み込み, duplicate フラグがあるドキュメントは除去し, lm_score でソートし(スコアが低い(品質が高い)順), 
25600 ドキュメントごとに jsonl.zstd を作り, 128 ファイル単位で chunk ごとに保存します.
1 ファイルは概ね 30 MB 程度になります.

lm_score ソートはドキュメント全体で行うため, 一度すべての jsonl.zstd を読み込むため, メモリ消費が大きいです(128 GB 以上)

## TODO

- out-of-core でソートしてメモリ使用量を減らす. 
- 現状は doc/lm_score/dedup json のマージは行数でしか判断していないため, 各ファイルに id を付与してより reliable にする(clean 時点で id をアサイン)
