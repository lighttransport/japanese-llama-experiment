# Based onTinyLlama: https://github.com/jzhang38/TinyLlama
# We include a simple full-parameter finetuning & inference script here. Our V0.1 chat model is finetuned using this script. 
# The FT dataset we use is oasst1-89k-ja

# V0.1
# Run on single GPU
# --multi-tpu --num_processes 2 --main_process_port 12345
CUDA_VISIBLE_DEVICES=0 accelerate launch finetune.py \
    --model_name_or_path ../10_incremental_pretrain/Japanese-TinyLlama-1.1B-1.0T/ \
    --output_dir ./output/jp-tinyllama-1T_FT_lr1e-5_ep5 \
    --logging_steps 10 \
    --save_strategy epoch \
    --data_seed 42 \
    --save_total_limit 6 \
    --evaluation_strategy epoch \
    --eval_dataset_size 512 \
    --max_eval_samples 1000 \
    --per_device_eval_batch_size 1 \
    --max_new_tokens 32 \
    --dataloader_num_workers 3 \
    --group_by_length=False \
    --logging_strategy steps \
    --remove_unused_columns False \
    --do_train \
    --do_eval \
    --warmup_ratio 0.05 \
    --lr_scheduler_type constant \
    --dataset oasst1-ja \
    --source_max_len 16 \
    --target_max_len 512 \
    --per_device_train_batch_size 4 \
    --max_steps 0 \
    --num_train_epochs 5 \
    --learning_rate 1e-5 \
    --adam_beta2 0.999 \
    --max_grad_norm 1.0 \
    --weight_decay 0.0 \
    --seed 0 \
    --trust_remote_code 
    #--report_to wandb 


