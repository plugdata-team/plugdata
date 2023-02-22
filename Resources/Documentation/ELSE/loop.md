---
title: loop

description: loop counter

categories:
 - object

pdcategory: ELSE, Triggers and Clocks, Data Management

arguments:
- type: float
  description: <f> — number of iterations, <f f f> — start, end, step
  default: 1
- type: float
  description: sets the end value (optional)
- type: float
  description: sets a counter step value
  default: 1

inlets:
  1st:
  - type: float
    description: sets number of iterations and starts the loop
  - type: list
    description: sets range start/end and step (optionally) and run loop
  - type: bang
    description: starts the loop
  2nd:
  - type: float
    description: sets number of iterations

outlets:
  1st:
  - type: float/bang
    description: counter output or bangs in bang mode

flags:
  - name: -offset <float>
    description: sets offset value (default 0)
  - name: -step <float>
    description: sets counter step value (default 1)
  - name: -b <float>
    description: sets to bang mode

methods:
  - type: offset <float>
    description: sets the starting value of the counter (for float)
  - type: set <list>
    description: a float sets number of iterations, a list sets start/end and step (optionally)
  - type: step <float>
    description: sets the increment value of the counter
  - type: range <list>
    description: sets start/end values
  - type: pause
    description: stops the loop
  - type: continue
    description: continues the loop after being paused

draft: false
---

[loop] is a loop counter, but it can also loop bangs like [until] (using the -b flag). A float sets number of count iteration upwards. A list with two elements sets a start and end value (and allows counting upwards or downwards). A counter step value can be set with a third optional element in a list.

