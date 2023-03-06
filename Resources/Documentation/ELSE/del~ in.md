---
title: del~ in

description: delay line input

categories:
 - object

pdcategory: ELSE, Effects, Buffers

arguments:
- type: symbol
  description: sets delay line name
  default: internal name relative to patch
- type: float
  description: delay size in ms or samples
  default: 1 sample

inlets:
  1st:
  - type: signal
    description: signal input into the delay line

outlets:
  1st:
  - type: signal
    description: dummy outlet to force order of execution

flags:
  - name: -samps
    description: sets time value to samples (default is ms)

methods:
  - type: clear
    description: clears delay line
  - type: size <float>
    description: changes the delay size
  - type: freeze <float>
    description: non-0 freezes, zero unfreezes

draft: false
---

Define a delay line and write to it. If neither "in" or "out" is specified as the first argument, the default it [del~ in].

