---
title: remove

description: remove elements from a list

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: list
  description: elements to remove
  default: 0

inlets:
  1st:
  - description: input list
    type: list
  2nd:
  - description: elements to remove
    type: list
  - description: clear elements to remove
    type: bang

draft: false
---

[remove] removes one or more number elements from a list. The right inlet or arguments set the elements to remove.
