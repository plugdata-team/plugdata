---
title: ratio2cents~

description: rational/Cents conversion (for signals)

categories:
 - object

pdcategory: ELSE, Signal Math, Converters

arguments:

inlets: 
  1st:
  - type: signal
    description: ratio value

outlets:
  1st:
  - type: signal
    description: converted cents value

draft: false
---

Use [ratio2cents~] to convert a signal representing a rational interval to an interval in cents. The conversion formula is;
cents = log2(ratio) * 1200
