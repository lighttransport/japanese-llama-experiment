import os

from tokenizers import Tokenizer
from tokenizers.models import Unigram
from tokenizers import normalizers
from tokenizers.trainers import UnigramTrainer
from tokenizers.pre_tokenizers import Whitespace
from datasets import load_dataset

os.makedirs('../data', exist_ok=True)

dataset_name = "range3/wikipedia-ja-20230101"

tokenizer = Tokenizer(Unigram())
tokenizer.pre_tokenizer = Whitespace()
tokenizer.normalizer = normalizers.NFKC()

# TODO: Use [BOS], [EOS] instead of [CLS], [SEP]?
# NOTE: Chinese LLaMa uses vocab_size=20000
# NOTE: 21000 or less fails to tokenize for wikipedia ja dataset
trainer = UnigramTrainer(special_tokens=["[UNK]", "[CLS]", "[SEP]", "[PAD]", "[MASK]"], unk_token="<UNK>", vocab_size=21500)

dataset = load_dataset(dataset_name)

def dataset_iter():
    # for cc100-ja
    # roughly 700MB
    # reducing `skip` will cause OOM if you have less than 128 GB CPU mem.
    # skip=100

    # for wikipedia
    skip=1
    for i in range(0, len(dataset['train']), skip):
        yield dataset['train'][i]['text']

tokenizer.train_from_iterator(dataset_iter(), trainer)

tokenizer.save('../data/tokenizer-wikipedia-ja.json')

