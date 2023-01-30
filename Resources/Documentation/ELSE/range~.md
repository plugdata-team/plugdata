---
title: range~

description: analyze range

categories:
 - object

pdcategory: ELSE, Analysis

arguments:

inlets:
  1st:
  - type: float/signal
    description: input signal to analyze
  - type: bang
    description: resets minimum and maximum values
  2nd:
  - type: float/signal
    description: impulse resets at signal rate

outlets:
  1st:
  - type: signal
    description: minimum range 
  2nd:
  - type: signal
    description: maximum range


draft: false
---

[range~] analyzes and outputs the signal's range (minimum and maximum) values.
