from tokenizers import Tokenizer
from tokenizers.models import BPE
from tokenizers.trainers import BpeTrainer
from tokenizers.pre_tokenizers import Whitespace
from datasets import load_dataset

tokenizer = Tokenizer(BPE(unk_token="[UNK]"))
tokenizer.pre_tokenizer = Whitespace()

trainer = BpeTrainer(special_tokens=["[UNK]", "[CLS]", "[SEP]", "[PAD]", "[MASK]"], vocab_size=30000)

#files = [f"data/wikitext-103-raw/wiki.{split}.raw" for split in ["test", "train", "valid"]]
#files = ['Good_Contents.txt', 'Featured_Contents.txt']

#dataset = load_dataset("wikitext", 'wikitext-2-raw-v1')
dataset = load_dataset('range3/cc100-ja')

def dataset_iter():
    skip=100
    for i in range(0, len(dataset['train']), skip):
        yield dataset['train'][i]['text']

#print(dataset['text'])
tokenizer.train_from_iterator(dataset_iter(), trainer)
tokenizer.save('data/tokenizer-cc100-ja.json')
#print(dataset)

