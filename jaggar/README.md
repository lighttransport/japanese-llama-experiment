# Jagger - C++ implementation of Pattern-based Japanese Morphological Analyzer

https://www.tkl.iis.u-tokyo.ac.jp/~ynaga/jagger/

## Build

```
$ wget http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/jagger/jagger-latest.tar.gz

$ tar xvf jaggar-latest.tar.gz
$ cd jagger-YYYY-MM-DD

# Download mecab-jumandic-7.0-20130310.tar.gz
# https://drive.google.com/drive/folders/0B4y35FiV1wh7fjQ5SkJETEJEYzlqcUY4WUlpZmR4dDlJMWI5ZUlXN2xZN2s2b0pqT3hMbTQ

$ tar zxvf mecab-jumandic-7.0-20130310.tar.gz

# Apply patch to mecab-jumandic
$ wget http://www.tkl.iis.u-tokyo.ac.jp/~ynaga/jagger/mecab-jumandic-7.0-20130310.patch
$ patch -p0 < mecab-jumandic-7.0-20130310.patch

# 京都大学ウェブ文書リード文コーパス
$ git clone git clone https://github.com/ku-nlp/KWDLC

# configure the build
$ ./configure

$ make model-benchmark
```

