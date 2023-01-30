---
title: slice

description: split lists

categories:
 - object

pdcategory: ELSE, Data Management 

arguments:
- type: float
  description: split point
  default: 0

inlets:
  1st:
  - type: list
    description: incoming list to split
  2nd:
  - type: float
    description: split point

outlets:
  1st:
  - type: list
    description: the left part of the split list
  2nd:
  - type: list
    description: the right part of the split list

draft: false
---

[slice] splits lists. The 'n' split point is set via argument or right inlet. If you slice at 'n', the left outlet spits the first 'n' elements and the right outlet sends the remaining elements. If you slice at '-n', the right outlet spits the last 'n' elements and the left outlet sends the first elements. If the number of elements is less than the split point, the whole list is output to the left if 'n' is positive, and to the right if negative. Symbols and floats are also taken.
