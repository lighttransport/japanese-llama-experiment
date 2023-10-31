import torch

filepath="output_dir/checkpoint-1200/pt_lora_model/adapter_model.bin"

model = torch.load(filepath, map_location="cpu")
for param in model:
    # model[param]: Tensor
    print(param, model[param].size())
