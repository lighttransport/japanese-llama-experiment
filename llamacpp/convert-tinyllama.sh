# if you downloaded model using huggingface transformers' from_pretrained.
#python convert-tinyllama.py ~/.cache/huggingface/hub/models--PY007--TinyLlama-1.1B-intermediate-step-715k-1.5T/snapshots/f14887da55a7a910d244de391ec05a05d963fc26 --outtype f16 --outfile tinyllama-1.1B-1.5T.gguf

# if you git clone the model.
python convert-tinyllama.py ../10_incremental_pretrain/Japanese-TinyLlama-1.1B-1.0T/ --outtype f16 --outfile Japanese-TinyLlama-1.1B-1.0T.gguf
