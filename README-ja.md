# 日本語 LLaMa 追加学習のためのスクリプト

* 日本語データセット構築(クリーニングと dedup(重複除去))
* 日本語データセットで既存の英語ベースの pretain model に対して追加事前学習
  * Chinese LLaMa を参考にしています https://github.com/ymcui/Chinese-LLaMA-Alpaca

の二本立てで構成されています.

## 背景

LLM(Large Language Model) のフルの学習(事前学習, pretrain)では, 品質の高いデータセットで学習させることが重要となっています
(as of 2023/07 この傾向ついては今後研究の発展により変わる可能性はあります).

特に文章の品質が高く, また重複や繰り返しが無いデータセットを用意するのが重要になります

* RefinedWeb https://arxiv.org/abs/2306.01116
* SlimPajama https://www.cerebras.net/blog/slimpajama-a-627b-token-cleaned-and-deduplicated-version-of-redpajama

## クリーニング済み日本語データセット構築

huggingface datasets にクリーニング済みですぐに追加事前学習に使えるのを用意しています.

https://huggingface.co/datasets/lighttransport/japanese-dataset-cleaned-experiment

## TODO

* 追加学習ではなく, SlimPajama と組み合わせ一から事前学習する
  * => あとから日本語知識を付け足すよりは性能がよくなるはず...(学習のコストはかかるが)
 
## License

MIT or Apache 2.0.

japanese-llama-experiment のスクリプトは, ソースコードの SPDX License identifier が明示的に無い場合は, MIT になります.

### Third party licenses

それぞれのファイルを参照ください.
