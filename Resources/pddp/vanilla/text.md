---
title: text
description: manage a list of messages
categories:
- object
pdcategory: Misc
last_update: '0.49'
see_also:
- list
- array
- scalar
- text define
- text get
- text set
- text insert
- text delete
- text size
- text tolist
- text fromlist
- text search
- text sequence
arguments:
- description: 'sets the function of [text], possible values: define, get, set, insert,
    delete, size, tolist, fromlist, search and sequence. The default value is ''define''.'
  type: symbol
flags:
- description: saves/keeps the contents of the text with the patch.
  flag: -k
inlets:
  1st:
  - type: bang
    description: output a pointer to the scalar containing the text.
  - type: clear
    description: clear contents of the text.
  - type: send <symbol>
    description: send pointer to a named receive object
  - type: read <symbol>
    description: read from a file (with optional -c flag).
  - type: write <symbol>
    description: write to a file (with optional -c flag).
  - type: sort
    description: sort the text contents.
  - type: click
    description: open text window.
  - type: close
    description: closes the text window.
outlets:
  1st:
  - type: pointer
    description: a pointer to the scalar containing the array.
  2nd:
  - type: anything
    description: outputs "updated" when text changes.
draft: false
---

