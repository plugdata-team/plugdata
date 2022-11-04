---
title: wavetable~

description: Wavetable oscillator

categories:
 - object

pdcategory: Audio Oscillators And Tables

arguments:
  1st:
  - type: symbol
    description: array name (optional)
    default: none
  2nd:
  - type: float
    description: sets frequency in Hz
    default: 0
  3rd:
  - type: float
    description: sets phase offset
    default: 0
  
inlets:
  1st:
  - type: float/signal
    description: sets frequency in hertz
  - type: set <symbol>
    description: sets an entire array to be used as a waveform
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

draft: false
---

[wavetable~] is an interpolating wavetable oscillator like Pd Vanilla's [tabosc4~]. It accepts negative frequencies, has inlets for phase sync and phase modulation.