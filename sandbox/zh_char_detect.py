# https://qiita.com/ry_2718/items/47c21792d7bbd3fe33b9
def is_zh(in_str):
    """
    SJISに変換して文字数が減れば簡体字があるので中国語
    """
    return (set(in_str) - set(in_str.encode('sjis','ignore').decode('sjis'))) != set([])


print(is_zh("现代汉语常用字表"))
print(is_zh("図"))
print(is_zh("圖")) # JIS第2水準にあり(`図`の旧字)
print(is_zh("图"))


# この後残ったものに対して, 新字体への変換, 異体字の正規化をするとよいだろう
