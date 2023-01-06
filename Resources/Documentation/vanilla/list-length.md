---
title: list length
description: manipulate lists
categories:
- object
pdcategory: General
see_also:
- list
- list append
- list prepend
- list store
- list split
- list trim
- list fromsymbol
- list tosymbol
inlets:
  1st:
  - type: anything
    description: messages to have its elements counted.
outlets:
  1st:
  - type: float
    description: list length.
draft: false
---
Number of items in list.

The "list length" object outputs the number of arguments in a list or other message.
