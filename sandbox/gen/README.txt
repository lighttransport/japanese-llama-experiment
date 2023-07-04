rinna-3.6b で日本語文章を作成.

## Requirements

* pytorch

## TODO

### ctranslate2 version


```
!ct2-transformers-converter \
    --model rinna/japanese-gpt-neox-3.6b \
    --quantization int8 \
    --output_dir ./rinna-3.6b
```


EoL.
