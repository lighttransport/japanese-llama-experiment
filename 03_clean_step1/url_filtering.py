from tld import get_tld

blacklist = ["cn", "ru", "tv", "info"]

def filter_url(url: str):
    try:
        tld_name = get_tld(url)
        if tld_name in blacklist:
            return False

        return True
    except Exception as exc:
        print(exc)
        return False


if __name__ == '__main__':
    print(filter_url("http://www.hello.cn/bora.index"))
    print(filter_url("http://dora.muda.ru/aaaa/bora.index"))
    print(filter_url("http://dora.muda.info/ccc"))
    print(filter_url("http://www.muda.com/dd"))
