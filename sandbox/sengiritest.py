# pip install sengiri

import sengiri

text ="""【 つれづれ。。。 】 「トライやる・ウィーク」によって,なんだか新鮮な気持ちで お仕事ができるようになったソウルライフレコード店長の小杉です。 おそらくご存知無い方も多いかもしれませんね, 「トライやる・ウィーク」。 ...
"""
print(sengiri.tokenize(text))

text ="""吾輩は猫で
ある
名前はまだない
"""
print(sengiri.tokenize(text))

