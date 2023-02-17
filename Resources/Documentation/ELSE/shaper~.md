---
title: shaper~

description: waveshaper

categories:
 - object

pdcategory: ELSE, Effects

arguments:
- type: anything
  description: array/table name/list of floats
  default: 1

flags:
- name: -dc
  description: DC offset for internal transfer function (default 0)
- name: -norm <f>
  description: normalization on <1> (default) or off <0>
- name: -fill <f>
  description: DC filter on <1> (default) or off <0>

inlets: 
  1st:
  - type: signal
    description: signal to be used as index

outlets:
  1st:
  - type: signal
    description: output of transfer function (waveshaping)

methods:
  - type: set <symbol>
    description: array/table name to be used for lookup
  - type: list
    description: harmonic weights for internal transfer function
  - type: dc <f>
    description: DC offset for internal transfer function
  - type: norm <f>
    description: normalization for internal function on <1> or off <0>
  - type: filter <f>
    description: - normalization for internal function on <1> or off <0>

draft: false
---

[shaper~] performs waveshaping with transfer functions, in which signal input values (from -1 to 1) are mapped to the the transfer function's indexes. Values outside the -1 to 1 range are wrapped inside it.
You can set an array for [shaper~], as in this example, or use its internal transfer function for Chebyshev polynomials.
