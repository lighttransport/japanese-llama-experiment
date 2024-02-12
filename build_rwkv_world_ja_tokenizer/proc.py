import json
import csv
from collections import deque

# assume emoji > 2.0
import emoji
from kaomoji.kaomoji import Kaomoji

emotion_csv_file = "Emoticon.csv"
kaomoji_txt_file = "kaomoji-list.txt"
input_json_file = "../models/rwkv_vocab_v20230424-ja.json"
output_json_file = "../models/rwkv_vocab_v20230424-ja-emo-kao.json"

kaomoji_dict = {}

with open(emotion_csv_file, "r", encoding="utf-8") as f:
    reader = csv.reader(f)
    for row in reader:
        kaomoji_dict[row[0]] = row[0]

with open(kaomoji_txt_file, "r", encoding="utf-8") as f:
    lines = f.readlines()
    for line in lines:
        kao = line.split()
        if len(kao):
            kao = kao[0]
            if not kao in kaomoji_dict:
                kaomoji_dict[kao] = kao


print(len(kaomoji_dict))


js = json.loads(open(input_json_file, "r", encoding="utf-8").read())
print(len(js))

id_to_str = {}

def list_available_ids(id_to_str, from_id = 1024, to_id = 65530):
    lst = deque()
    for i in range(from_id, to_id):
        if not i in id_to_str:
            lst.append(i)

    return lst


# id -> string
for k, v in js.items():
    id_to_str[v] = k

# ensure 128 ~ 256 is not present
for i in range(128, 257):
    assert not i in id_to_str, "{} should not appear in vocab".format(i)


available_ids = list_available_ids(id_to_str)

# add emoji character
for e in emoji.EMOJI_DATA.keys():
    if not e in js:

        e_id = available_ids.popleft()

        js[e] = e_id
        id_to_str[e_id] = e
        
# add kaomoji
for k in kaomoji_dict.keys():
    if not k in js:

        k_id = available_ids.popleft()

        js[k] = k_id
        id_to_str[k_id] = k
    
print("The number of ids remain after adding emoji&kamoji", len(available_ids))
print("nvocab after adding emoji&kamoji", len(js))

# TODO: sort by length?
with open(output_json_file, "w") as f:
    f.write(json.dumps(js, indent=2, ensure_ascii=True))

