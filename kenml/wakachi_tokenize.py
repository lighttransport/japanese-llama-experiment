from fugashi import Tagger
import unicodedata
import time
import tqdm

tagger = Tagger('-Owakati')

lines = open("wiki.txt").readlines()

wf = open('wiki-nfkc-wakachi.txt', 'w')

s = time.time()
for line in tqdm.tqdm(lines):
	if not line.strip().endswith("。"):
		# ignore line not ends with zenkaku-maru(Kuten)
		continue

	normalized_line = unicodedata.normalize('NFKC', line)
	ret = tagger.parse(normalized_line)
	wf.write(ret + "\n")
e = time.time()
print("normalization + wakachi-gaki time: ", (e - s), " [secs]")
