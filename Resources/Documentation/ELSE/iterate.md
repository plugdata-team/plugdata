---
title: iterate

description: iterate through a list

categories:
- object

pdcategory: ELSE, Data Management

arguments:
- description: direction, >= 0 is left to right, < 0 is reverse
  type: float
  default: 0

inlets:
  1st:
  - type: bang
    description: outputs the last received input as sequential elements
  - type: anything
    description: split elements sequentially
  2nd:
  - type: float
    description: sets direction >= 0 is left to right, < 0 is reverse

outlets:
  1st:
  - type: float/symbol
    description: according to the input element, in sequential order

flags:
  - name: -trim
    description: trims symbol selector on the output

draft: false
---

[iterate] splits a message to individual elements (floats/symbols) and outputs them sequentially.

