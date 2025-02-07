---
title: gatehold

description: hold gate values

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: hold time in ms
  default: (0, no hold)

inlets:
  1st:
  - type: float
    description: gate value
  2nd:
  - type: float
    description: hold time in ms

outlets:
  1st:
  - type: float
    description: held gate

draft: false
---
[gatehold] holds a gate value for a certain amount of time (specified in ms) after the gate has closed. If a new gate starts before the hold time is finished, the object is retriggered.
