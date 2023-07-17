# reject if `text` contains 10%(threshold) or more ascii characters
def filter_ascii(text: str, threshold=10):
    n = len(text)

    nratio = threshold
    nascii = 0
    for i in text:
        code = ord(i)
        if code < 128:
            nascii += 1

            if (100 * float(nascii) / (float(n))) > float(nratio):
                return False

    return True

if __name__ == '__main__':
    print(filter_ascii("aaaaaaaaa今日"))
    print(filter_ascii("a今日明日漁ってしあさってはカレーです"))
        
