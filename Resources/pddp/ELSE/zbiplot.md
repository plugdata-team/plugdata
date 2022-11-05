---
title: zbiplot

description: Z-plane Biquad plot

categories:
- object

pdcategory: GUI

arguments: (none)

inlets:
  1st:
  - type: coef <list>
    description: list of coefficients as taken by [biquad~]
  - type: list
  - description: list of poles coordinates
  2nd:
  - type: list
    description: list of zeros coordinates

outlets: (none)

draft: false
---

[zbiplot] plots the biquad coefficients as poles and zeros on the Z-Plane.
