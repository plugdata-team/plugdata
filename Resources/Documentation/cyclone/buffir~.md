---
title: buffir~
description: convolve with a buffer
categories:
 - object
pdcategory: cyclone, Buffers
arguments:
- type: symbol
  description: table name
- type: float
  description: table offset
  default: 0
- type: float
  description: table size/length (maximum is 4096)
  default: 0
inlets:
  1st:
  - type: signal
    description: the signal to be convolved with samples from the buffer
  2nd:
  - type: float/signal
    description: offset (samples)
  3rd:
  - type: float/signal
    description: size/length (samples) - maximum is 4096
outlets:
  1st:
  - type: signal
    description: the filtered signal

methods:
  - type: clear
    description: clears current input history
  - type: set <list>
    description: set <buffer/table name, offset, size> (resets arguments)

---

[buffir~] is a table/buffer based FIR (finite impulse response) filter. An input signal is convolved with 'n' samples of a buffer.

