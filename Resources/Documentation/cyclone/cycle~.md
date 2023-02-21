---
title: cycle~
description: wavetable oscillator
categories:
 - object
pdcategory: cyclone, Signal Generators
arguments:
- type: float
  description: sets frequency in Hz
  default: 0
- type: symbol
  description: sets array used as the wavetable (but only reads 512 points from the buffer)
  default: internal cosine table
- type: float
  description: sets offset within the given array
  default: 0
inlets:
  1st:
  - type: float/signal
    description: sets frequency in Hz (negative values allowed)
  2nd:
  - type: float/signal
    description: phase offset
outlets:
  1st:
  - type: signal
    description: a periodically repeating waveform

flags:
  - name: @frequency <float>
  - name: @phase <float>
  - name: @buffer <symbol>
  - name: @buffer_offset <float>
  - name: @buffer_sizeinsamps <float>

methods:
  - type: frequency <float>
    description: sets frequency in Hz
  - type: phase <float>
    description: sets phase offset
  - type: set
    description: only a "set" message sets to the default cosine wave
  - type: set <symbol, float>
    description: sets array name and offset (reads only 512 points)
  - type: setall <symbol>
    description: sets an array using its entire length
  - type: buffer <symbol>
    description: sets array (analogous to the setall message)
  - type: buffer_offset <float>
    description: sets offset into buffer
  - type: buffer_sizeinsamps <f>
    description: changes the buffer size to powers of 2 (16 to 65536)

draft: false
---

[cycle~] is a linear interpolating oscillator* that reads repeatedly through one cycle of a waveform. The default internal waveform is one cycle of a cosine wave (16k in size, 64 bits), but you can set other waveforms from given arrays.

