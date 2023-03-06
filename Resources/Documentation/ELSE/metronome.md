---
title: metronome

description: high level metronome

categories:
 - object

pdcategory: ELSE, Triggers and Clocks

arguments:
- type: float
  description: tempo in BPM 
  default: 120

inlets:
  1st:
  - type: bang
    description: start or restart metronome
  - type: float
    description: non-0 (re)starts, zero stops
  2nd:
  - type: float
    description: set tempo value in BPM

outlets:
  1st:
  - type: bang
    description: bang at every beat
  2nd:
  - type: list
    description: bar, sub-bar, beat, sub-beat count
  3rd:
  - type: float
    description: beat phase (0-1)
  4th:
  - type: list
    description: actual beat length, actual tempo and bar duration in ms

flags:
  - name: -name <symbol>
    description: set a clock name
  - name: -beat <symbol>
    description: set a beat length
  - name: -sub
    description: sets to subdivision (subtempo) mode

methods:
  - type: start
    description: start or restart metronome
  - type: stop
    description: stop metronome
  - type: pause
    description: pause metronome
  - type: continue
    description: continue if paused
  - type: beat <symbol>
    description: set a beat length
  - type: timesig <sym, f>
    description: set time signature symbol and group value
  - type: sub <float>
    description: sets to subdivision (subtempo) mode
  - type: tempo <float>
    description: set tempo value in BPM

draft: false
---

[metronome] understands complex time signatures and outputs timeline data (bar, sub-bar, beat and sub-beat count) and phase output. It also syncs to [clock] objects.
