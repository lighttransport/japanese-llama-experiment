from tld import get_tld

# TLD で除去
# (機械翻訳の可能性が高い)
blacklist = ["cn", "ru", "tv", "hu", "pl", "info"]

# まとめ系サイトを除去
matome_list = ["2ch.", "5ch.", "matome", "jin115", "115jin", "doorblog.jp", "umasoku.com", "fc2.com", "sokuhou", "huyosoku.com", "kumusoku", "kabu-sokuhou.com", "soku"]

# 詐欺的サイトを除去
scam_list = ["paolabaertl.com", "ojiyama.com", "anpiacenza.org", "rojobermelo.com", "giscountry.com"
,"occupyapec.com"
,"testedevelocidadegvt.com"
,"buy-anabolic-steroid.biz"
,"recette-tunisienne.com"
]


def filter_url(url: str):
    try:
        tld_name = get_tld(url)
        if tld_name in blacklist:
            return True

    except Exception as exc:
        print(exc)
        # invalid URL(or direct IP address). classified as invalid.
        return True


    for m in matome_list:
        if m in url:
            return True

    for s in scam_list:
        if s in url:
            return True

    # ok.
    return False


if __name__ == '__main__':
    print(filter_url("http://www.hello.cn/bora.index"))
    print(filter_url("http://dora.muda.ru/aaaa/bora.index"))
    print(filter_url("http://dora.muda.info/ccc"))
    print(filter_url("http://www.muda.com/dd"))
    print(filter_url("http://2ch.net/ee"))
    print(filter_url("http://akibamatome.com/ee"))
