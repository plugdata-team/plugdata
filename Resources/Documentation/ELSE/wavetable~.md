---
title: wavetable~

description: wavetable oscillator

categories:
 - object

pdcategory: ELSE, Signal Generators, Arrays and Tables, Buffers

arguments:
  - type: symbol
    description: array name (optional)
    default: none
  - type: float
    description: sets frequency in Hz
    default: 0
  - type: float
    description: sets phase offset
    default: 0
  
inlets:
  1st:
  - type: float/signal
    description: sets frequency in Hz

  2nd:
  - type: float/signal
    description: phase sync (resets internal phase)
  3rd:
  - type: float/signal
    description: phase offset (modulation input)
    
outlets:
  1st:
  - type: signal
    description: a periodically repeating waveform

methods: 
  - type: set <symbol>
    description: sets an entire array to be used as a waveform


draft: false
---

[wavetable~] is an interpolating wavetable oscillator like Pd Vanilla's [tabosc4~]. It accepts negative frequencies, has inlets for phase sync and phase modulation.
