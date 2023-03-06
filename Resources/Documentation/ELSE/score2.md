---
title: score2

description: score sequencer variant

categories:
- object

pdcategory: ELSE, Triggers and Clocks

arguments:
- type: symbol
  description: score file to load
  default: none

click: clicking on the object opens data/edit window (same as 'open')

inlets:
  1st:
  - type: start
    description: starts the sequence
  - type: stop
    description: stops the sequence
  - type: float
    description: non-0 starts, 0 stops sequence

outlets:
  1st:
  - type: anything
    description: event data
  2nd:
  - type: float
    description: event duration in ms
  3rd:
  - type: anything
    description: score data (bar number, tempo and other things by the user)
  4th:
  - type: bang
    description: when sequence is over

methods:
  - type: pause
    description: pauses playing
  - type: continue
    description: continues playing
  - type: show
    description: opens edit window
  - type: hind
    description: closes edit window
  - type: open <symbol>
    description: import score from a file, no symbol opens dialog window
  - type: save <symbol>
    description: write score to file, no symbol opens dialog window

draft: false
---

[score2] is a variant of [score] that has the same rhythmic notation syntax as [pattern].
