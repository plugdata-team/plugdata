---
title: pong~

description: signal range limiter (fold, wrap & clip)

categories:
 - object

pdcategory: cyclone, Signal Math

arguments:
- type: float
  description: 0 (fold), 1 (wrap), 2 (clip), 3 (none)
  default: 0
- type: float
  description: low range value
  default: 0
- type: float
  description: high range value
  default: 1

inlets:
  1st:
  - type: float/signal
    description: value to be limited into a given range
  2nd:
  - type: float/signal
    description: sets low range
  3rd:
  - type: float/signal
    description: sets high range

outlets:
  1st:
  - type: signal
    description: original, clipped, wrapped, or folded signal

flags:
  - name: @mode
    description: sets mode (<fold>, <wrap>, <clip> or <none>)
    default: 0
  - name: @range <f, f>
    description: sets low and high range values
    default: 0, 1

methods:
 - type: mode <float>
   description: 0 (fold), 1 (wrap), 2 (clip), 3 (none)
  - type: low <symbol>
    description: <fold>, <wrap>, <clip> or <none>
  - type: range <f, f>
    description: sets low and high range values

draft: true
---

Use [pong~] to fold, wrap or clip its input signal within a given low-high range.
