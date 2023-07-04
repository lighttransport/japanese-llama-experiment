import evaluate 
import datasets

input_texts = datasets.load_dataset("wikitext",
                                    "wikitext-2-raw-v1",
                                    split="test")["text"][:50]

perplexity = evaluate.load("perplexity", module_type="metric")


input_texts = [s for s in input_texts if s!='']
results = perplexity.compute(model_id='gpt2',
                             predictions=input_texts)

print(results["mean_perplexity"])

for i in range(len(results["perplexities"])):
    print(results["perplexities"][i], input_texts[i])
