---
title: knob

description: knob GUI

categories:
 - object

pdcategory: ELSE, UI

arguments:
inlets:
  1st:
  - type: float
    description: sets and outputs value
  - type: bang
    description: outputs value

outlets:
  1st:
  - type: float
    description: knob's value

flags:
  - name: -set <float>
    description: sets value (no output)
  - name: -load <float>
    description: sets load value, no float sets current
    default: 0
  - name: -size <float>
    description: sets size in pixels
    default: 50
  - name: -arc <float>
    description: non zero sets to arc drawing mode
    default: 1
  - name: -nosquare <float>
    description: don't draw square
    default: 0
  - name: -nooutline <float>
    description: don't draw outline
    default: 0
  - name: -bgcolor <list>
    description: sets background color in hexdecimal or RGB list
    default: #dfdfdf
  - name: -fgcolor <list>
    description: sets foreground color in hexdecimal or RGB list
    default: #000000
  - name: -circular <float>
    description: non zero sets to circular dragging mode
    default: 0
  - name: -angle <float>
    description: sets angular range value in degrees
    default: 320
  - name: -offset <float>
    description: sets angular offset in degrees
    default: 0
  - name: -send <symbol>
    description: sets send symbol
    default: empty
  - name: -receive <symbol>
    description: sets receive symbol
    default: empty
  - name: -exp <float>
    description: sets exponential value
    default: 1
  - name: -ticks <float>
    description: sets number of ticks
    default: 0
  - name: -discrete <float>
    description: non zero sets to discrete mode
    default: 0
  - name: -jump <float>
    description: non zero sets to jump on click mode
    default: 0

methods:
  - type: set <float>
    description: sets value (no output)
  - type: load <float>
    description: sets load value, no float sets current
    default: 0
  - type: size <float>
    description: sets size in pixels
    default: 50
  - type: arcstart <float>
    description: sets arc start value, no float sets current
  - type: arc <float>
    description: non zero sets to arc drawing mode
    default: 1
  - type: square <float>
    description: non zero sets to square drawing mode
    default: 0
  - type: bgcolor <list>
    description: sets background color in hexdecimal or RGB list
    default: #dfdfdf
  - type: fgcolor <list>
    description: sets foreground color in hexdecimal or RGB list
    default: #000000
  - type: circular <float>
    description: non zero sets to circular dragging mode
    default: 0
  - type: angle <float>
    description: sets angular range value in degrees
    default: 320
  - type: offset <float>
    description: sets angular offset in degrees
    default: 0
  - type: send <symbol>
    description: sets send symbol
    default: empty
  - type: receive <symbol>
    description: sets receive symbol
    default: empty
  - type: exp <float>
    description: sets exponential value
    default: 1
  - type: ticks <float>
    description: sets number of ticks
    default: 0
  - type: discrete <float>
    description: non zero sets to discrete mode
    default: 0
  - type: jump <float>
    description: non zero sets to jump on click mode
    default: 0
  - type: readonly <float>
    description: sets read-only mode
    default: 0
  - type: number <float>
    description: sets number mode
    default: 0
  - type: savestate <float>
    description: saves the current state
    default: 0
  - type: lb <float>
    description: set loadbang mode
    default: 0
  - type: numbersize <float>
    description: sets the size of the displayed number
    default: 0
  - type: numberpos <float, float>
    description: sets the position of the displayed number
    default: 0
  - type: active <float>
    description: activates or deactivates the knob
    default: 0
  - type: reset
    description: resets the knob to its default state
    default: 0
  - type: param <symbol>
    description: sets a parameter for the knob
    default: 0
  - type: var <symbol>
    description: assigns a variable to the knob
    default: 0
  - type: arccolor <list>
    description: sets the arc color of the knob
    default: 0
draft: false
---
