---
title: midi.clock

description: MIDI clock

categories:
- object

pdcategory: ELSE, MIDI

arguments:
- type: float
  description: sets BPM and also sets to internal clock
  default:

inlets:
  1st:
  - type: float
    description: sets BPM and also sets to internal clock

outlets:
  1st:
  - type: bang
    description: bang at each clock tick
  2nd:
  - type: float
    description: time interval in BPM from external source
    
methods:
  - type: external
    description: sets to external clock

draft: false
---

[midi.clock] is a MIDI clock abstraction. It works with an internal or external clock (from an external MIDI device). It sends a bang at each MIDI clock and also sends a time interval (in BPM) from external clock sources.
By default, [clock] is set to an external input, which comes from the MIDI device specified in Pd's MIDI configuration. But if you give it a float argument, it sets to internal clock at the BPM specified by the float.
