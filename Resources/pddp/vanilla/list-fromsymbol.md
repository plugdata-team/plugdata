---
title: list fromsymbol
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
- list length
- list tosymbol
inlets:
  1st:
  - type: symbol
    description: symbol to be converted to a list of character codes.
outlets:
  1st:
  - type: list
    description: list of converted character codes.
draft: false
---
Convert from symbols to lists of numeric character codes.

`[list fromsymbol]` and `[list tosymbol]` allow you to do string manipulations (such as scanning a filename for '/' characters). They convert a list of numbers (which might be ASCII or might be unicode if, for example, they represent a filename on a non-ASCII machine) to or from a symbol.
