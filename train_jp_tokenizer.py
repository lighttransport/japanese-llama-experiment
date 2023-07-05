# NOTE: 128 GB CPU mem is required.
from tokenizers import Tokenizer
from tokenizers.models import BPE
from tokenizers import normalizers
from tokenizers.trainers import BpeTrainer
from tokenizers.pre_tokenizers import Whitespace
from datasets import load_dataset

tokenizer = Tokenizer(BPE(unk_token="[UNK]"))
tokenizer.pre_tokenizer = Whitespace()
tokenizer.normalizer = normalizers.NFKC()

# TODO: Use [BOS], [EOS] instead of [CLS], [SEP]?
# NOTE: Chinese LLaMa uses vocab_size=20000
trainer = BpeTrainer(special_tokens=["[UNK]", "[CLS]", "[SEP]", "[PAD]", "[MASK]"], vocab_size=30000)

dataset = load_dataset('range3/cc100-ja')

def dataset_iter():
    # roughly 700MB
    # reducing `skip` will cause OOM if you have less than 128 GB CPU mem.
    skip=100
    for i in range(0, len(dataset['train']), skip):
        yield dataset['train'][i]['text']

tokenizer.train_from_iterator(dataset_iter(), trainer)
tokenizer.save('data/tokenizer-cc100-ja.json')

