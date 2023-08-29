import unicodedata
import time
import tqdm

lines = open("wiki.txt").readlines()

wf = open('wiki-nfkc-char.txt', 'w')

s = time.time()
for line in tqdm.tqdm(lines):
	if not line.strip().endswith("。"):
		# ignore line not ends with zenkaku-maru(Kuten)
		continue

	if len(line.strip()) == 0:
		continue

	normalized_line = unicodedata.normalize('NFKC', line.strip())
	ret = " ".join(normalized_line)
	wf.write(ret + "\n")
e = time.time()
print("normalization + char decomp time: ", (e - s), " [secs]")
