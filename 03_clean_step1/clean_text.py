def char_is_hiragana(c):
    return u'\u3040' <= c <= u'\u309F'

def contains_hiragana(s):
    return any(char_is_hiragana(c) for c in s)

def count_whitespaces(text):
    c = 0
    for i in text:
        if i == " ":
            c += 1

    return c

def is_closing_brace(c):
    chars = ["»", "」", "》", "´", ")", "）", ")", "〉", ">", "】", "]"]

    if c in chars:
        return True

    return False



def do_clean(text: str, ws_threshold=1):

    # 1. skip text if it does not contain any hiragana.
    if not contains_hiragana(text):
        return None

    # 2. remove sentence if it ends with "...", "," or no punctuation.
    sentences = text.split('\n') # split by '\n'
    results = []
    for sent in sentences:
        # 1. remove if the sentence contains some whitespaces
        if count_whitespaces(sent) >= ws_threshold:
            continue
        elif sent.endswith("..."):
            continue
        elif sent.endswith("... "):
            continue
        elif sent.endswith("...　"): # zenkaku space
            continue
        elif sent.endswith("."):
            pass
        elif sent.endswith("。"):
            pass
        elif sent.endswith(")"):
            # FIXME: May be ascii kaomoji :-)
            pass
        elif sent.endswith("!"):
            pass
        elif sent.endswith("！"):
            pass
        elif sent.endswith("?"):
            pass
        elif sent.endswith("？"):
            pass
        elif sent.endswith(","):
            continue
        elif sent.endswith("\""):
            continue
        elif sent.endswith("'"):
            continue
        elif is_closing_brace(sent[:-1])
            pass
        else:
            # assume sentence is broken.
            # TODO: Do nlp analysys 
            continue

        results.append(sent)

    return "\n".join(results)
