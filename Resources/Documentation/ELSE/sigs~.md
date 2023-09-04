---
title: sigs~

description: generate multichannel signals from lists

categories:
- object

pdcategory: ELSE, Signal Generators

arguments:
  - description: values to generate multichannel signal
    type: list
    default: 0 0

inlets:
  nth:
  - type: float
    description: number to convert to a channel signal

outlets:
  1st:
  - type: signals
    description: converted signal

draft: false
---

[sigs~] is like vanilla's [sig~] but generates multichannel signals from an argument list and creates separate inlets for each argument (minimum size is 2).

