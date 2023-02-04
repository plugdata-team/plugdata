---
title: spread

description: compare floats and spread them

categories:
 - object

pdcategory: ELSE, Data Management 

arguments:
- type: list
  description: floats to compare to
  default: 0

flags:
- name: -mode <f>
  description: sets mode, 0 (default) and 1 (in reverse to [moses])

inlets:
  1st:
  - type: float
    description: input value to spread

outlets:
  nth:
  - type: float
    description: output to an outlet as the result of the comparison

methods:
  - type: args <list>
    description: replace arguments
  - type: mode <f>
    description: sets mode

draft: false
---

Similar to [moses], [spread] compares an incoming float to the arguments and spreads to different outputs. One difference to [moses] is that it takes multiple arguments and has an extra mode.

