---
title: keyboard
description: keyboard GUI

categories:
 - object

pdcategory: ELSE, UI, Tuning

arguments:

inlets:
  1st:
  - type: float
    description: MIDI Note value
  - type: list <f,f>
    description: MIDI Note and Velocity values
  2nd:
  - type: float
    description: velocity of MIDI note value

outlets:
  1st:
  - type: list <f,f>
    description: MIDI Note and Velocity values

flags:
  - name: -width <float>
    description: width in pixels (default 17)
  - name: -height <float>
    description: height in pixels (default 80)
  - name: -oct <float>
    description: number of octaves (default 4)
  - name: -lowc <float>
    description: number of lowest C (default 3 - that is "C3")
  - name: -tgl
    description: sets to toggle mode (default non toggle)
  - name: -norm <float>
    description: velocity normalization value (default 0)
  - name: -send <symbol>
    description: sets send symbol (default 'empty')
  - name: -receive <symbol>
    description: sets receive symbol (default 'empty')

methods:
  - type: on <list>
    description: list MIDI Notes to be on with velocity of 127
  - type: off <list>
    description: list MIDI Notes to be off with velocity of 0
  - type: set <float, float>
    description: set note (pitch/velocity) without output
  - type: width <float>
    description: set key width
  - type: height <float>
    description: set keyboard height
  - type: oct <float>
    description: transpose octaves up or down
  - type: 8ves <float>
    description: set number of octaves
  - type: lowc <float>
    description: set number of lowest C (e.g. 4 = C4)
  - type: norm <float>
    description: set velocity normalization
  - type: toggle <float>
    description: sets toggle mode on <1> of off <0>
  - type: flush
    description: flushes hanging Note On keys
  - type: send <symbol>
    description: sets send symbol
  - type: receive <symbol>
    description: sets receive symbol

draft: false
---

[keyboard] is a GUI that receives MIDI notes and also generates them from mouse clicking. Right click it for properties!

