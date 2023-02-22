---
title: limit

description: limit message with time

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: initial time limit
  default: 0 - no limit
- type: float
  description: non-0 sets to ignore mode
  default: 0

inlets:
  1st:
  - type: anything
    description: any message input message to be speed limited
  2nd:
  - type: float
    description: changes the time limit (in ms)
outlets:
  1st:
  - type: anything
    description: the last input in the time interval after last output
  2nd:
  - type: anything
    description: ignored messages (in ignore mode)

draft: false
---

[limit] only allows messages through if a given time has elapsed since the previous input/output. Otherwise, it waits until that time passes and then sends the last received message since the previous output (ignoring the others).

