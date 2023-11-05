# Convert TinyLlama model

There is an issue in original llama.cpp's convert.py, so you should use convert-tinyllama.py in this repo
(convert-tinyllama.py in this repo applies the fix: https://github.com/jzhang38/TinyLlama/issues/24 )

## Use huggingface from_pretrained

Run `download_tinyllama.py`, then modify a folder containing download path to `convert-tinyllama.sh`,
then

```
$ bash convert-tinyllama.sh`
```

