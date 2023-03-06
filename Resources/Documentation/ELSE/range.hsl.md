---
title: range.hsl

description: range horizontal slider

categories:
- object

pdcategory: ELSE, UI

arguments:

inlets:
  1st:
  - type: bang
    description: outputs last value
  - type: list
    description: sets range values and outputs it

outlets:
  1st:
  - type: list
    description: min/max range values

flags:
  - name: -width <float>
    description:
  - name: -height <float>
    description:
  - name: -range <f, f>
    description:
  - name: -mode <float>
    description:
  - name: -bgcolor <f, f, f>
    description:
  - name: -fgcolor <f, f, f>
    description:
  - name: -send <symbol>
    description:
  - name: -receive <symbol>
    description:

methods:
  - type: set <f, f>
    description: sets range values
  - type: width <float>
    description: sets width size
  - type: height <float>
    description: sets height size
  - type: range <f, f>
    description: sets range values
  - type: mode <float>
    description: sets mode (0 - shift, 1 - select, 2 - expand, 3 - expand)
  - type: bgcolor <f, f, f>
    description: sets background color
  - type: fgcolor <f, f, f>
    description: sets foreground color
  - type: send <symbol>
    description: sets send symbol
  - type: receive <symbol>
    description: sets receive symbol
  

draft: false
---

[range.hsl] is a range horizontal slider GUI abstraction, (based on an object by .mmb). It has 4 modes of operation, see below.
