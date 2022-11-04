---
title: tempo~

description: Metronome

categories:
- object

pdcategory:

arguments:
  1st:
  - type: float
    description: bpm/hz/ms
    default: 0
  2nd:
  - type: float
    description: swing deviation in %
    default: 0

flags:
  - name: -on
    description: initially turn it on
  - name: -ms
    description: sets time measure to ms
    default: bpm
  - name: -hz
    description: sets time measure to hz
    default: bpm
  - name: -mul <float>
    description: sets multiplier
    default: 1
  - name: -seed <float>
    description: sets seed
    default: no float sets a unique internal

inlets:
  1st:
  - type: float/signal
    description: gate (on/off)
  - type: bang
    description: sync the metronome
  - type: mul <float>
    description: sets multiplier
  - type: ms <f, f>
    description: sets time to ms, optional floats set tempo and swing
  - type: hz <f, f>
    description: sets time to hz, optional floats set tempo and swing
  - type: bpm <f, f>
    description: sets time to bpm, optional floats set tempo and swing
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal
  2nd:
  - type: float/signal
    description: tempo in ms or bpm
  3rd:
  - type: float/signal
    description: swing deviation parameter (in %)
  4th:
  - type: float
    description: an impulse syncs the metronome

outlets:
  1st:
  - type: signal
    description: impulses at metronome beat

  
draft: false
---

The [tempo~] object is like [imp~], but sends impulses at a time specified in bpm as in a metronome and is turned on/off by a gate input.
