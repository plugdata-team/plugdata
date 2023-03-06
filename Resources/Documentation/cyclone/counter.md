---
title: counter
description: counts over a range
categories:
 - object
pdcategory: cyclone, Data Math
arguments:
- type: list
  description: <f>: max; <f,f>: min, max;  <f,f,f>: direction (0-up,1-down,2-updown), min, max
  default: direction = 0 (up), min = 0, max = 2^24
inlets:
  1st:
  - type: bang
    description: goes to the next value and outputs it
  - type: float
    description: same as bang
  2nd:
  - type: bang
    description: switches up/down direction
  - type: float
    description: sets direction, 0: up, 1: down, 2: updown (alternating)
  3rd:
  - type: bang
    description: sets counter to minimum value (no output)
  - type: float
    description: sets counter value with no output - if outside the range, it resets min/max according to compatmode (see help)
  4th:
  - type: bang
    description: sets counter to minimum value and outputs it
  - type: float
    description: sets counter value and outputs it - if outside the range, it resets min/max according to compatmode (see help)
  5th:
  - type: bang
    description: sets counter to maximum value and outputs it
  - type: float
    description: sets the maximum value (no output)
outlets:
  1st:
  - type: float
    description: the current value of the counter
  2nd:
  - type: bang
    description: when max is reached (if 'carrybang' is set & counting up)
  - type: float
    description: 1 when max is reached, 0 when next round starts (and 'carryint' is set and counting up)
  3rd:
  - type: bang
    description: when min is reached (if carrybang is set & counting down)
  - type: float
    description: 1 when min is reached, 0 when next round starts (if carryint is set and counting down)
  4th:
  - type: float
    description: counts how many times the counter reached the maximum

flags:
  - name: @carryflag
    description: 0 for carryint (default) / 1 for carrybang
  - name: @compatmode
    description: 0 for current (default) / 1 for ancient (see help)

methods:
  - type: carrybang
    description: sets to "carrybang" mode: sends bangs at count rounds
  - type: carryint
    description: sets to "carryint" mode (default): sends "1" at the end of a count round, "0" at the start of the next round
  - type: dec
    description: decrements counter by 1 (regardless of the direction)
  - type: down
    description: sets direction to downwards
  - type: flags <list>
    description: 1st float sets 'carryflag': 0 (carryint), 1 (carrybang). 2nd sets 'compatmode' mode: 0 (current), 1 (ancient)
  - type: goto <float>
    description: same as 'set' message
  - type: inc
    description: increments counter by 1 (regardless of the direction)
  - type: jam <float>
    description: sets counter to that value and outputs it - values outside the range are ignored
  - type: next
    description: outputs next value (same as bang)
  - type: max <float>
    description: sets maximum value (doesn't output it)
  - type: min <float>
    description: sets minimum value and outputs it
  - type: set <float>
    description: sets counter's next value (considering direction) - no output
  - type: setmin <float>
    description: sets minimum value (doesn't output it)
  - type: state
    description: (in any inlet) prints current settings state on the Pd window
  - type: up
    description: sets counter direction to upwards
  - type: updown
    description: sets direction to shift between up/down when reaching limits
  - type: compatmode <f>
    description: sets @compatmode attribute: (0 = current, 1 = ancient)
  - type: carryflag <f>
    description: sets @carryflag attribute: (0 = carryint, 1 = carrybang)

draft: false
---

[counter] counts up and/or down by '1' in a given range.

