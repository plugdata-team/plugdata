---
title: dec2hex

description: convert decimal to hexadecimal

categories:
- object

pdcategory: ELSE, Data Math, Converters

arguments:
- description: decimal value(s) to convert
  type: float/list
  default: 0

inlets:
  1st:
  - type: float/list
    description: decimal value(s) to convert (outputs the result)
  - type: bang
    description: convert and output
  2nd:
  - type: float/list
    description: decimal value(s) to convert (at bangs)

outlets:
  1st:
  - type: symbol
    description: the converted hexadecimal value

draft: false
---

[dec2hex] converts decimal values to hexadecimal ones. The output is preceded by "0x" and the letters in the hexadecimal values are upper case. You can also convert lists. Only floats are converted, symbols are ignored.

