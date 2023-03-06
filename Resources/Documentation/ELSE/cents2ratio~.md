---
title: cents2ratio~

description: cents/rational conversion for signals

categories:
 - object

pdcategory: ELSE, Tuning, Converters

arguments:
inlets: 
  1st:
  - type: signal
    description: value in cents

outlets:
  1st:
  - type: signal
    description: converted ratio value

draft: false
---

Use [cents2ratio~] to convert a signal representing an interval in cents to an interval represented as a decimal rational number. The conversion formula is;
ratio = pow(2, (cents/1200))
