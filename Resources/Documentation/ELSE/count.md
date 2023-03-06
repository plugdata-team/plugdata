---
title: count

description: a counter

categories:
- object

pdcategory: ELSE, Data Math

arguments:
- description: 1 float sets max / 2 floats set min & max
  type: list

inlets:
  1st:
  - type: bang
    description: counts
  - type: float
    description: sets new count value and outputs it
  2nd:
  - type: list
    description: updates arguments

outlets:
  1st:
  - type: float
    description: count value
  2nd:
  - type: bang
    description: bang when reaching count limits

flags:
  - name: -alt
    description: sets to alternating mode
  - name: reset <float>
    description: sets start & reset value

methods:
  - type: set <float>
    description: sets new count value
  - type: reset <float>
    description: without a float, resets to the reset point, but a float sets a new reset point as well
  - type: max <float>
    description: sets maximum value
  - type: min <float>
    description: sets minimum value
  - type: up
    description: sets count direction upwards
  - type: down
    description: sets count direction downwards
  - type: alt <float>
    description: non-0 sets to alternating mode (between up/down)

draft: false
---

[count] counts and wraps between a minimum and maximum value. It has modes for counting up, down and alternating between them.
