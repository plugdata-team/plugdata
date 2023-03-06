---
title: delete

description: delete elements from message

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: element index to delete from
  default: 0 - none
- type: float
  description: number of elements to delete
  default: 1

inlets:
  1st:
  - type: anything
    description: input message
  2nd:
  - type: float
    description: element number
  3rd:
  - type: float
    description: number of elements to delete

outlets:
  1st:
  - type: list
    description: remaining elements from the input message
  2nd:
  - type: list/bang
    description: deleted elements or bang when deleting fails

draft: false
---

[delete] deletes one or more elements from an input message. The mid inlet or 1st argument sets the element index and the right inlet or 2nd argument sets number of elements to delete. Right outlet outputs deleted item(s) and left outlet the remaining elements from the list. Right outlet sends a bang if deleting fails (if index and/or n elements are out of range). An index of 0 means no items are deleted!. Negative indexes count backwards (-1 is the last element and so on).

