import ginza
import sys, os
import unicodedata

#input_file = "normalize_test.txt"

#
# Rules.
#
# - [x] Normalize text(e.g. hankaku-kana to zenkaku-kana). Use NFKC mode.
# - [x] Remove a sentence ends with "..."
# - [ ] Remove a sentence too short.
# - [ ] Replace numbers to all zero
#   - 今日のドル円は 100 円 23 銭  -> 0 円 0 銭
#

def main():
    if len(sys.argv) < 2:
        print("Need input.txt")
    
    input_filename = sys.argv[1]

    if len(sys.argv) > 2:
        output_filename = sys.argv[2]
    else:
        output_filename = os.path.splitext(input_filename)[0] + "-filtered.txt"

    wf = open(output_filename, "w", encoding="utf-8")

    with open(input_filename, "r", encoding="utf-8") as f:
        lines = f.readlines()
        print("read lines", len(lines))
        for line in lines:
            nline = unicodedata.normalize('NFKC', line.rstrip())

            if nline.endswith("..."):
                print(nline)
                continue

            #print(nline)
            pass

    

if __name__ == '__main__':
    main()

            

