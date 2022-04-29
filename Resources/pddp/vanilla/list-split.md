---
title: list split
description: manipulate lists
categories:
- object
pdcategory: General
see_also:
- list
- list append
- list prepend
- list store
- list trim
- list length
- list fromsymbol
- list tosymbol
arguments:
- description: initialize split point.
  type: float
inlets:
  1st:
  - type: anything
    description: messages to be split into smaller lists.
  2nd:
  - type: float
    description: sets new 'n' split point.
outlets:
  1st:
  - type: list
    description: if incoming list is shorter than n.
draft: false
---
Cut a list into smaller ones.

The "list split" object takes lists and outputs the first "n" items (left outlet) and the remaining ones (middle outlet). If the incoming list also has 'n' items, the middle outlet spits a list with zero elements (which becomes a bang). The two outputs appear in the usual right-to-left order. In case there are fewer than "n" items in the list, it is output (in its entirety) from the third outlet instead. The creation argument or the inlet sets the split point.
