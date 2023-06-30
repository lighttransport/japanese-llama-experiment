from transformers import LlamaTokenizer
from sentencepiece import sentencepiece_model_pb2 as sp_pb2_model
import sentencepiece as spm
import argparse

japanese_tokenizer = args.japanese_tokenizer
#chinese_sp_model_file = args.chinese_sp_model_file

llama_tokenizer_model_name = "openlm-research/open_llama_3b"
# load
llama_tokenizer = LlamaTokenizer.from_pretrained(llama_tokenizer_model_name)
print(llama_tokenizer)
