# pip install wtpsplit

from wtpsplit import WtP

wtp = WtP("wtp-canine-s-12l")
# optionally run on GPU for better performance
# also supports TPUs via e.g. wtp.to("xla:0"), in that case pass `pad_last_batch=True` to wtp.split
# wtp.half().to("cuda")

text ="""【 つれづれ... 】 「トライやる・ウィーク」によって,なんだか新鮮な気持ちで お仕事ができるようになったソウルライフレコード店長の小杉です. おそらくご存知無い方も多いかもしれませんね, 「トライやる・ウィーク」. ...
"""

threshold = wtp.get_threshold("ja", "ud")
print(threshold)
ret = wtp.split(text, lang_code='ja', threshold=threshold)
print(ret)

prob = wtp.predict_proba(text, lang_code='ja', style='ud')
print("prob", prob)


