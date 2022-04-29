---
title: snapshot~
description: convert a signal to a number on demand
categories:
- object
see_also:
- sig~
pdcategory: General Audio Manipulation
last_update: '0.37'
inlets:
  1st:
  - type: bang
    description: convert a signal to a float.
  - type: set <float>
    description: set a float value for the next DSP block.
outlets:
  1st:
  - type: float
    description: the converted signal at every bang.
draft: false
---
The snapshot~ object takes a signal and converts it to a control value whenever it receives a bang in its left outlet. This object is particularly useful for monitoring outputs.

A 'set' message is provided for the (rare) situations where you might make a known change to the signal input, and then read snapshot's value before any ensuing signal computation.