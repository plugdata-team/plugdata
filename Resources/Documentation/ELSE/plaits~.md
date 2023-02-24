---
title: plaits~

description: Plaits module from Mutable Instruments

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:
  - type: float
    description: pitch
    default: 0
  - type: float
    description: harmonics
    default: 0
  - type: float
    description: timbre
    default: 0
  - type: float
    description: morph
    default: 0
  - type: float
    description: cutoff
    default: 0.5
  - type: float
    description: decay
    default: 0.5

flags:
  - name: -cv/-midi/-voct
    description: set to pitch input in CV, MIDI, or v/oct (default Hz)
  - name: -model <float>
    description: set model number (default 0)
  - name: -trigger
    description: set to trigger mode (default regular)

inlets:
  1st:
  - type: float/signal
    description: pitch input
  - type: bang
    description: control trigger (when in trigger mode)
  2nd:
  - type: signal
    description: signal trigger input
  3rd:
  - type: signal
    description: level for built-in VCA or excitation signal

outlets:
  1st:
  - type: signal
    description: regular signal output
  2nd:
  - type: signal
    description: secondary (auxiliary) signal output
  3rd:
  - type: anything
    description: output info on setting model or 'dump'

methods:
  - type: hz
    description: set frequency mode to Hz
  - type: midi
    description: set frequency mode to MIDI
  - type: cv
    description: set frequency mode to CV
  - type: voct
    description: set frequency mode to v/oct
  - type: trigger <float>
    description:  non-0 sets to trigger mode
  - type: model <float>
    description: set model number (0-15)
  - type: harmonics <float>
    description: set harmonics (0-1)
  - type: timbre <float>
    description: set timbre (0-1)
  - type: morph <float>
    description: set morph (0-1)
  - type: cutoff <float>
    description: set lowpass gate cutoff/color (0-1)
  - type: decay <float>
    description: set lowpass gate decay (0-1)
  - type: print
    description: output information on terminal window
  - type: dump
    description: output information on rightmost outlet

draft: false
---
[plaits~] is based on the "Plaits" module from Mutable Instruments.

