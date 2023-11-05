# modified by Light Transport Entertainment Inc.
# 运行脚本前请仔细阅读wiki(https://github.com/ymcui/Chinese-LLaMA-Alpaca-2/wiki/pt_scripts_zh)
# Read the wiki(https://github.com/ymcui/Chinese-LLaMA-Alpaca-2/wiki/pt_scripts_zh) carefully before running the script
lr=2e-4

# 7B
# blocksize=512
#lora_rank=64

# tinyllama 1.1B
# blocksize=2048
lora_rank=256

lora_alpha=128

lora_trainable="q_proj,v_proj,k_proj,o_proj,gate_proj,down_proj,up_proj"
modules_to_save="embed_tokens,lm_head"
lora_dropout=0.05

pretrained_model="PY007/TinyLlama-1.1B-intermediate-step-480k-1T"
japanese_tokenizer_path=../data/merged_tokenizer_hf
dataset_dir="lighttransport/Japanese-CharShu-59B"
data_cache=temp_data_cache_dir

# NOTE: 8 works on 3090x2(24GB x 2)
per_device_train_batch_size=8

gradient_accumulation_steps=8
block_size=2048
output_dir=output_dir

#deepspeed_config_file=ds_zero2_no_offload.json
deepspeed_config_file=ds_zero1_no_offload.json

ngpus_per_node=2
use_flash_attn=True

torchrun --nnodes 1 --nproc_per_node ${ngpus_per_node} incr_pretrain_tinyllama.py \
    --deepspeed ${deepspeed_config_file} \
    --model_name_or_path ${pretrained_model} \
    --tokenizer_name_or_path ${japanese_tokenizer_path} \
    --dataset_dir ${dataset_dir} \
    --data_cache_dir ${data_cache} \
    --validation_split_percentage 0.001 \
    --per_device_train_batch_size ${per_device_train_batch_size} \
    --do_train \
    --seed $RANDOM \
    --fp16 \
    --num_train_epochs 1 \
    --lr_scheduler_type cosine \
    --learning_rate ${lr} \
    --warmup_ratio 0.05 \
    --weight_decay 0.01 \
    --logging_strategy steps \
    --logging_steps 10 \
    --save_strategy steps \
    --save_total_limit 3 \
    --save_steps 200 \
    --gradient_accumulation_steps ${gradient_accumulation_steps} \
    --preprocessing_num_workers 16 \
    --block_size ${block_size} \
    --output_dir ${output_dir} \
    --overwrite_output_dir \
    --ddp_timeout 30000 \
    --logging_first_step True \
    --lora_rank ${lora_rank} \
    --lora_alpha ${lora_alpha} \
    --trainable ${lora_trainable} \
    --lora_dropout ${lora_dropout} \
    --modules_to_save ${modules_to_save} \
    --torch_dtype float16 \
    --load_in_kbits 16 \
    --gradient_checkpointing \
    --flash_attn ${use_flash_attn} \
    --ddp_find_unused_parameters False
