---
title: count

description: A counter

categories:
- object

pdcategory: Math Functions

arguments:
- description: 1 float sets max / 2 floats set min & max
  type: list

inlets:
  1st:
  - type: bang
    description: counts
  - type: float
    description: sets new count value and outputs it
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
    description: non-zero sets to alternating mode (between up/down)
  2nd:
  - type: list
    description: updates arguments

outlets:
  1st:
  - type: float
    description: count value
  1st:
  - type: bang
    description: bang when reaching count limits

flags:
  - name: -alt
    description: sets to alternating mode
  - name: reset <float>
    description: sets start & reset value

draft: false
---

[count] counts and wraps between a minimum and maximum value. It has modes for counting up, down and alternating between them.