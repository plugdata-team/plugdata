---
title: dbgain~

description: adjust gain to a dB level

categories:
- object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: dB level adjustment
  default: 0

inlets:
  1st:
  - type: signals
    description: input value(s) to adjust gain
  2nd:
  - type: signals
    description: db value(s)
outlets:
  1st:
  - type: signals
    description: gain adjusted input

draft: false
---

The [dbgain~] object is basically like [level~], but without the GUI. One difference is that it allows multichannels on the right inlet to set different dB levels.
