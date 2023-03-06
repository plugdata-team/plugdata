---
title: separate

description: separate symbols

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: symbol
  description: sets the separator character
  default: "space"

inlets:
  1st:
  - type: symbol/anything
    description: the symbol to be converted to a list
  2nd:
  - type: symbol
    description: sets separator symbol

outlets:
  1st:
  - type: list
    description: list with separated elements from a symbol

draft: false
---

[separate] allows you to separate a symbol into a list with different elements by setting the character that separates one element from the others. The default separator is a "space".
