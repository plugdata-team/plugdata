---
title: float2bits
description: convert float to bits

categories:
 - object

pdcategory: ELSE, Data Math, Converters

arguments:
- type: float
  description: sets float to convert to bits

inlets:
  1st:
  - type: float
    description: sets float value and converts to bits
  - type: bang
    description: converts to bits
  2nd:
  - type: float
    description: sets float to convert to bits

outlets:
  1st:
  - type: anything
    description: a list of bits when receiving a bang or "signal", "exponent" and "mantissa" values when receiving a 'split' message

methods:
  - type: set <float>
    description: sets float value and doesn't convert it
  - type: split
    description: sends "mantissa", "exponent" and "signal" values separately

draft: false
---

[float2bits] converts decimal float (IEEE754 Single precision 32-bit) values to binary form (a list of 32 bits). By the way, this is the type of float used in Pure Data.

