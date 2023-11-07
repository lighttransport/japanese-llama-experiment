# Based on Chinese LLaMa
# https://github.com/ymcui/Chinese-LLaMA-Alpaca
import os
os.environ["PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION"]="python"
from transformers import LlamaTokenizer
from tokenizers import Tokenizer
from sentencepiece import sentencepiece_model_pb2 as sp_pb2_model
import sentencepiece as spm
import argparse
import unicodedata
parser = argparse.ArgumentParser()

#llama_model = 'openlm-research/open_llama_13b' # default download from huggingface.
llama_model = "PY007/TinyLlama-1.1B-intermediate-step-240k-503b" # tinyllama 1.1B

#japanese_model = 'tokenizer-cc100-ja.json' # from a file(huggingface tokenizers JSON format).
japanese_model = '../data/tokenizer-wikipedia-ja.json' # from a file(huggingface tokenizers JSON format).
parser.add_argument('--llama_tokenizer_dir', default=llama_model, type=str)
parser.add_argument('--japanese_tokenizer_file', default=japanese_model, type=str)
#parser.add_argument('--japanese_sp_model_file', default='./japanese_sp.model', type=str)
args = parser.parse_args()

llama_tokenizer_dir = args.llama_tokenizer_dir
japanese_tokenizer_file = args.japanese_tokenizer_file
#japanese_sp_model_file = args.japanese_sp_model_file

# load
llama_tokenizer = LlamaTokenizer.from_pretrained(llama_tokenizer_dir)

japanese_tokenizer = Tokenizer.from_file(japanese_tokenizer_file)
# or load sp model. 
#japanese_sp_model = spm.SentencePieceProcessor()
#japanese_sp_model.Load(japanese_sp_model_file)

llama_spm = sp_pb2_model.ModelProto()
llama_spm.ParseFromString(llama_tokenizer.sp_model.serialized_model_proto())
japanese_spm = sp_pb2_model.ModelProto()
#japanese_spm.ParseFromString(japanese_tokenizer.sp_model.serialized_model_proto())
#japanese_spm.ParseFromString(japanese_sp_model.serialized_model_proto())

# print number of tokens
print(len(llama_tokenizer))
#print(len(japanese_sp_model))
print(llama_tokenizer.all_special_tokens)
print(llama_tokenizer.all_special_ids)
print(llama_tokenizer.special_tokens_map)

## Add Chinese tokens to LLaMA tokenizer
llama_spm_tokens_set=set(p.piece for p in llama_spm.pieces)
print(len(llama_spm_tokens_set))
print(f"Before:{len(llama_spm_tokens_set)}")


jp_vocab = japanese_tokenizer.get_vocab() # Dict[str, int]

for vocab in jp_vocab.keys():
    piece = vocab
    if piece not in llama_spm_tokens_set:
        new_p = sp_pb2_model.ModelProto().SentencePiece()
        new_p.piece = piece
        new_p.score = 0
        llama_spm.pieces.append(new_p)
print(f"New model pieces: {len(llama_spm.pieces)}")

#for p in japanese_spm.pieces:
#    piece = p.piece
#    print(piece)
#    if piece not in llama_spm_tokens_set:
#        new_p = sp_pb2_model.ModelProto().SentencePiece()
#        new_p.piece = piece
#        new_p.score = 0
#        llama_spm.pieces.append(new_p)
#print(f"New model pieces: {len(llama_spm.pieces)}")

## Save
output_sp_dir = 'merged_tokenizer_sp'
output_hf_dir = 'merged_tokenizer_hf' # the path to save Chinese-LLaMA tokenizer
os.makedirs(output_sp_dir,exist_ok=True)
with open(output_sp_dir+'/japanese_llama.model', 'wb') as f:
    f.write(llama_spm.SerializeToString())
tokenizer = LlamaTokenizer(vocab_file=output_sp_dir+'/japanese_llama.model')

tokenizer.save_pretrained(output_hf_dir)
print(f"Chinese-LLaMA tokenizer has been saved to {output_hf_dir}")


# Test
llama_tokenizer = LlamaTokenizer.from_pretrained(llama_tokenizer_dir)
japanese_llama_tokenizer = LlamaTokenizer.from_pretrained(output_hf_dir)
print(tokenizer.all_special_tokens)
print(tokenizer.all_special_ids)
print(tokenizer.special_tokens_map)
text='''吾輩は猫である。ﾜｶﾞﾊｲ は㈱である.
The primary use of LLaMA is research on large language models, including'''

print("Test text:\n",text)
norm_text = unicodedata.normalize('NFKC', text)
print("Normalized text:\n", norm_text)
print(f"Tokenized by LLaMA tokenizer:{llama_tokenizer.tokenize(norm_text)}")
print(f"Tokenized by Japanese-LLaMA tokenizer:{japanese_llama_tokenizer.tokenize(norm_text)}")
