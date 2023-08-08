from datasets import load_dataset


dataset = load_dataset("oscar-corpus/OSCAR-2301",
                        use_auth_token=True, # required
                        language="ja", 
                        #streaming=True, # optional
                        split="train") # optional

