---
title: cents2ratio~

description: Cents/Rational conversion (for signals)

categories:
 - object

pdcategory: Math (Conversion)

arguments: none

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

Use [cents2ratio~] to convert a signal representing an interval in cents to an interval respresented as a decmal rational number. The conversion formula is;
ratio = pow(2, (cents/1200))