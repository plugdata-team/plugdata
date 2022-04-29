---
title: text define
description: create, store, and/or edit texts
categories:
- object
pdcategory: Misc
last_update: '0.49'
see_also:
- list
- array
- scalar
- text
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
- description: set text name.
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
aliases:
- text
---
"text define" maintains a text object and can name it so that other objects can find it (and later should have some alternative, anonymous way to be found).

an optional `-c` flag allows you to read or write to/from a file interpreting carriage returns as separators.
