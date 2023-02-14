---
title: tempo

description: metronome

categories:
- object

pdcategory: ELSE, Triggers and Clocks

arguments:
  - type: float
    description: bpm/Hz/ms
    default: 0
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
    description: sets time measure to Hz
    default: bpm
  - name: -mul <float>
    description: sets tempo subdivision
    default: 1
  - name: -seed <float>
    description: sets seed
    default: no float sets a unique internal

inlets:
  1st:
  - type: float
    description: toggle (on/off)
  - type: bang
    description: sync the metronome
  2nd:
  - type: float
    description: tempo in ms, Hz or bpm
  3rd:
  - type: float
    description: swing deviation parameter (in %)

outlets:
  1st:
  - type: bang
    description: swing deviation parameter (in %)

methods:
  - type: start
    description: turn the metronome on
  - type: mul <float>
    description: sets tempo subdivision
  - type: stop
    description: turn the metronome off
  - type: ms <f, f>
    description: sets time to ms, optional floats set tempo and swing
  - type: hz <f, f>
    description: sets time to Hz, optional floats set tempo and swing
  - type: bpm <f, f>
    description: sets time to bpm, optional floats set tempo and swing
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

The [tempo] object sends bang at a time specified in bpm or ms. It has a swing parameter and is turned on and off by a toggle.
