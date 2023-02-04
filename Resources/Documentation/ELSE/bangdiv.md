---
title: bangdiv

description: bang divider

categories:
- object

pdcategory: ELSE, Triggers and Clocks

arguments:
  - description: divisor value
    type: float
    default: 1
  - description: start count value
    type: float
    default: 0

inlets:
  1st:
  - type: bang
    description: bang signal to be divided
  2nd:
  - type: float
    description: sets the divisor value (minimum=1)

outlets:
  1st:
  - type: bang
    description: first bang
  2nd:
  - type: bang
    description: subsequent bangs until divisor value is hit

methods:
  - type: div <float>
    description: sets the divisor value
  - type: start <float>
    description: sets the start value
  - type: reset
    description: resets the counter to the start value

draft: false
---

[bangdiv] outputs bangs when receiving bangs by sorting them based on a corresponding divisor value. The first bang in the count is output to the left outlet, while subsequent bangs are sent to the right outlet until the divisor value is hit, starting the cycle again. A starting value lets you start in the middle of a count. If the start value is negative, it adds that many counts to the first time a bang is output.
