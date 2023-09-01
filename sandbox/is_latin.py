# Basic Latin + Lain-1 supplement
# https://cloford.com/resources/charcodes/utf-8_latin.htm
g_latin = list(range(ord("!"), ord("~")+1))+list(range(ord("¡"), ord("¬")+1))+list(range(ord("®"), ord("ÿ")+1))

def is_latin(d: int):
    if d in g_latin:
        return True
    return False

txs = ["The world", "日本語death", "ëgg"]

for text in txs:
    for s in text:
        print(s, is_latin(ord(s)))

#text = "The world"

