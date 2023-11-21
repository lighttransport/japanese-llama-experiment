# %%

import json
import pandas as pd
from typing import Dict, List

# %%
j = json.loads(open("oasst1_89k_ja.json").read())
print(len(j))

# %%

def reconstruct_conversation_rec(data: List, trees: Dict, parent_id: int):

  nodes = []

  msg_id = data[parent_id]['message_id']

  if len(trees[msg_id]) == 0:

    # leaf
    return [[data[parent_id]]]

  # build conversation tree as list
  # => [parent, child, grandchild, ...]

  # Use the first one
  child_id = trees[msg_id][0]
  node = []
  node.append(data[parent_id])
  
  child_nodes = reconstruct_conversation_rec(data, trees, child_id)
  
  if child_nodes is not None:

    # Choose the first one for non-root response.
    # TODO: use multiple responses or choose the best response
    for c in child_nodes[0]:
      node.append(c)

  return [node]

def reconstruct_conversation(data: List, trees: Dict, root_id: int):

  node = []

  responses = reconstruct_conversation_rec(data, trees, root_id)
  assert responses is not None
  
  # Use the first one
  node = responses[0]

  return node
# %%
# simple tree struct 
# key = message_id
trees = {}

for d in j:
  # content = children
  trees[d['message_id']] = []

root_ids = [] # ids

for i, d in enumerate(j):
  if d['parent_id'] == 'nan':
    root_ids.append(i)
  else:
    # append to parent
    trees[d['parent_id']].append(i)

print(len(root_ids))


# %%

conversations = []
for root_id in root_ids: 
  conversation = reconstruct_conversation(j, trees, root_id)

  # ignore conversation which does not end with 'assistant'
  if conversation[-1]['role'] != 'assistant':
    continue

  conversations.append(conversation)
# %%
# %%

outj = json.dumps(conversations, indent=2)
# %%
with open("oasst1-ja-chat-multiturn.json", 'w') as f:
  f.write(outj)
