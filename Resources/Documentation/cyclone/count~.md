---
title: count~
description: counter at the sample rate
categories:
 - object
pdcategory: cyclone, Signal Math
arguments:
- type: float
  description: minimum value
  default: 0
- type: float
  description: maximum value to loop at
  default: 0 - no loop
- type: float
  description: specify if it's initially off <0> or on <1>
  default: 0
- type: float
  description: autoreset on <1> or off <0>
  default: 0
inlets:
  1st:
  - type: bang
    description: triggers counter and start counting from minimum
  - type: float
    description: triggers counter and start counting from the given number
  - type: list
    description: up to 4 floats that resets the arguments
  - type: signal
    description: 0 to non-0 transition starts counter. Non-0 to 0 stops it
  2nd:
  - type: float
    description: sets maximum value to loop at (0 eliminates the maximum)
outlets:
  1st:
  - type: signal
    description: outputs minimum value or count values when triggered

flags:
  - name: @autoreset <float>
    description: 1 (autoreset enabled) 0 (disabled)

methods:
  - type: stop
    description: stops the counter and resets to minimum
  - type: autoreset <1/0>
    description: <1> enables and <0> disables autoreset
  - type: min <float>
    description: sets minimum value without triggering the counter
  - type: set <f, f>
    description: sets minimum and maximum values without triggering

draft: false
---

[count~] outputs a signal increasing by 1 for each sample elapsed. It starts at a given minimum and it can loop at a given maximum value (which is actually that value - 1).

