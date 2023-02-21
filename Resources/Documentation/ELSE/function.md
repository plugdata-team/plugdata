---
title: function
description: function GUI

categories:
 - object

pdcategory: ELSE, UI, Data Math, Envelopes and LFOs

arguments:

inlets:
  1st:
  - type: float
    description: values from 0-1 are treated as indexes 
  - type: list
    description: sets and outputs breakpoints function
  - type: bang
    description: outputs breakpoints function

outlets:
  1st:
  - type: list
    description: breakpoints function for [envgen~]/[function~]

flags:
  - name: -height <float>
    description: (default 100)
  - name: -width <float>
    description: (default 200)
  - name: -send <symbol>
    description: (default none)
  - name: -receive <symbol>
    description: (default none)
  - name: -bgcolor <f, f, f>
    description: 
  - name: -fgcolor <f, f, f>
    description: 
  - name: -resize <float>
    description: 
  - name: -min <float>
    description: (default 0)
  - name: -max <float>
    description: (default 1)
  - name: -init
    description: 
  - name: -set <list>
    description: (default 0 1000 0)

methods:
  - type: i <float>
    description: indexes of table (function value is output)
  - type: set <list>
    description: sets breakpoints function
  - type: duration <float>
    description: resets overall duration
  - type: min <float>
    description: sets minimum graph boundary
  - type: max <float>
    description: sets maximum graph boundary
  - type: resize <float>
    description: sets new graph bounds according to min/max
  - type: height <float>
    description: sets height
  - type: width <float>
    description: sets width
  - type: fgcolor <f, f, f>
    description: sets RGB color of foreground
  - type: bgcolor <f, f, f>
    description: sets RGB color of background
  - type: send <symbol>
    description: sets send symbol
  - type: receive <symbol>
    description: sets receive symbol
  - type: init <float>
    description: non-0 sets to init mode

draft: false
---

[function] is a breakpoints function GUI, mainly used with [function~] or [envgen~]. You can click and drag on it or send it lists.

