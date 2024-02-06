# based on...
# https://qiita.com/e-a-st/items/8a9a791f302f03414f38

import json
import xml.etree.ElementTree as ET

filepath = "jawiki-20240201-pages-articles-multistream.xml"

xmlctx = ET.iterparse(filepath, events=('start', 'end'))

count = 0

title = ""

jsonl = []

for event, elem in xmlctx:
    if event == 'start' and elem.text:
        # Assume 'title', then 'text'
        if elem.tag.endswith("title"):
            title = elem.text

            elem.clear()
            continue

        elif elem.tag.endswith("text"):
            d = {}
            category = ""
            for line in elem.text.splitlines():
                if line.startswith("[[Category:"):
                    category += line

            if len(category) == 0:
                elem.clear()
                continue

            d["title"] = title
            d["category"] = category
            d["id"] = count # TODO: use wikipedia id?

            jsonl.append(json.dumps(d, ensure_ascii=False))

            del d

            if (count % 10000) == 0:
                print("Processed {} items...".format(count))
                
        else:
            # free memory to save memory pressure
            elem.clear()

            continue

    else:
        # free memory to save memory pressure
        elem.clear()

        continue

    # free memory to save memory pressure
    elem.clear()

    count += 1

with open("title_and_category.jsonl", "w") as f:
    f.write('\n'.join(jsonl))

print(count)

