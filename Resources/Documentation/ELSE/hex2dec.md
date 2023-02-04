---
title: hex2dec

description: convert hexadecimal to decimal

categories:
- object

pdcategory: ELSE, Data Math, Converters

arguments:
- description: hexadecimal values to convert
  type: anything
  default: 0x0

inlets:
  1st:
  - type: anything
    description: hexadecimal value(s) to convert (outputs the result)
  - type: bang
    description: convert and output, or output the last converted value(s)
  2nd:
  - type: anything
    description: hexadecimal value to convert when receiving a bang

outlets:
  1st:
  - type: list
    description: the converted decimal value(s)

draft: false
---

[hex2dec] converts hexadecimal values to decimal ones. It takes symbols whose format allows lower and upper case letters and can also include "0x" or "0X" prefixes (necessary in the case of lists and arguments to avoid confusion with floats, which are ignored). List and symbol selectors aren't necessary.

