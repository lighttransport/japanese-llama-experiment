from transformers import LlamaForCausalLM, LlamaTokenizer
from transformers.trainer_utils import get_last_checkpoint


checkpoint_dir="../10_incremental_pretrain/output_dir/"

last_checkpoint = get_last_checkpoint(checkpoint_dir)
if last_checkpoint is None and len(os.listdir(checkpoint_dir)) > 0:
    raise ValueError(
        f"Output directory ({checkpoint_dir}) already exists and is not empty. "
        "Use --overwrite_output_dir to overcome."
    )

print(last_checkpoint)
