from datasets import load_dataset, DatasetDict

dataset = load_dataset("lighttransport/Japanese-CharShu-59B", num_proc=8)

# We choose 1% each(2% in total), based on SlimPajama
# https://github.com/Cerebras/modelzoo/blob/97bdaf4460ace1681ad437b07ba33f0e179f5ca4/modelzoo/transformers/data_processing/slimpajama/preprocessing/shuffle_holdout.py#L110

test_and_validate_percentage = 0.02

# https://discuss.huggingface.co/t/how-to-split-main-dataset-into-train-dev-test-as-datasetdict/1090/7

# 2% for validate & test
train_testvalid = dataset['train'].train_test_split(test_size=test_and_validate_percentage)

test_valid = train_testvalid['test'].train_test_split(test_size=0.5)

train_test_valid_dataset = DatasetDict(
    { 'train': train_testvalid['train'],
      'validate': test_valid['test'],
      'test': test_valid['train']
    })

# TODO: ensure 'test' dataset is not included in 'train' split

print(train_test_valid_dataset)


