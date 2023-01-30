---
title: replace

description: replace element from a message

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: set item index to replace
  default: 0
- type: float/symbol
  description: number
  default: 0

inlets:
  1st:
  - type: anything
    description: input message
  2nd:
    - type: float
      description: element index
  3rd:
    - type: float/symbol
      description: new element

outlets:
  1st:
  - type: list
    description: list with replaced element


draft: false
---

[replace] replaces an element from an input message according to an element number defined by the 1st argument or mid inlet. Elements are indexed from 1, a 0 value means no element is replaced and negative values count from last to the first element. Element numbers get clipped to the first and last items of the input message. The right inlet or 2nd argument sets the new element.
