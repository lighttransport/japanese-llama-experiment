from sentencepiece import sentencepiece_model_pb2 as sp_pb2_model
import sentencepiece as spm

sp_model_file = "rinna-3.6b-tokenizer.json/spiece.model"

sp_model = spm.SentencePieceProcessor()
sp_model.Load(sp_model_file)


spm = sp_pb2_model.ModelProto()
spm.ParseFromString(sp_model.serialized_model_proto())

print(spm)
