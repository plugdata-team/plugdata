---
title: clock

description: synced clocks

categories:
- object

pdcategory: ELSE, Triggers and Clocks

arguments:
  - description: (optional) clock name
  type: symbol
  default: internal local to patch
  - description: BPM for main clock, or divider for synced clocks
  type: float/symbol
  default: 120 for main clock, 1 for synced clocks

inlets:
  1st:
  - type: float
    description: non-0 starts the clock, zero stops it
  - type: bang
    description: resyncs the clock
  2nd:
  - type: float
    description: BPM for main clock or multiplier for synced clock
  - type: symbol 
    description: multiplier fraction for synced clocks

outlets:
  1st:
  - type: bang
    description: bang according to clock tempo

flags:
  - name: -s
  description: set to synced clock (default main)

methods:
  - type: beat <float>
    description: sets beat length

draft: false
---

A [clock] object sends bangs at a regular tempo and controls other synced [clock] objects with a relative tempo.
By default, [clock] works as a main clock and you can set it to 'synced' with the '-s' flag. A synced clock is controlled by the main clock, so it only works when the main clock is on. A synced clock also has a relative tempo in relation to the main clock tempo (as in a clock multiplier) and can be synced to a [metronome].
You should have only one main clock, but you can have several clocks synced to this main one. By default, clock gets local names according to the patch window, but you can set a different name so you can have different clocks named differently.
Below we have a main clock at 60 BPM and a synced clock that divider the tempo to a factor of two (so it plays twice as fast).
