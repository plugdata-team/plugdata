---
title: pvoc.player~

description: multi-channel phase vocoded player

categories:
- object

pdcategory: ELSE, Buffers

arguments:
- description: (optional) number of channels (max 64)
  type: float
  default: 1 if no file is given, or sound file's if given
- description: the name of the file to open
  type: symbol
  default: none
- description: autostart <1: on, 0: off>
  type: float
  default: 0
- description: loop <1: on, 0: off>
  type: float
  default: 0

inlets:
  1st:
  - type: bang
    description: (re)start playing the buffer from the beginning
  - type: float
    description: <1> is the same as "start", <0> is the same as "stop"
  2nd:
  - type: float
    description: sets playing speed
  3rd:
  - type: float
    description: sets transposition in cents

outlets:
  nth:
  - type: signal
    description: the buffer signal of the corresponding channel
  2nd:
  - type: bang
    description: a bang when finishing playing the buffer

flags:
  - name: -speed <float>
    description: sets playing speed (default 100)
  - name: -transp <float>
    description: sets transposition in cents (default 0)
  - name: -loop
    description: turns loop mode on (default off)
  - name: -range <float, float>
    description: sets sample range (default: 0 1)

methods:
  - type: open <symbol>
    description: opens a file with the symbol name (no symbol opens dialog box) and starts playing
  - type: start
    description: same as bang
  - type: stop
    description: stops and goes back to the beginning
  - type: loop <float>
    description: loop on <1> or off <0>
  - type: range <f, f>
    description: sets sample's playing range (from 0 to 1)
  - type: pause
    description: pauses playing the buffer
  - type: reload
    description: reloads the file into the buffer and starts playing
  - type: set <symbol>
    description: sets a file to open (needs a reload message)
  - type: continue
    description: unpauses and continues playing the buffer

draft: false
---

[pvoc.player~] is like [player~] but provides independent time stretching and pitch shifting via a phase vocoder. NOTE: THIS OBJECT IS HEAVY ON THE CPU!!!

