---
title: uzi

description: bang loop/counter

categories:
 - object

pdcategory: cyclone, Data Management

arguments:
- type: float
  description: number of bangs
  default: 0
- type: float
  description: counter starting point (offset)
  default: 1

inlets:
  1st:
  - type: float
    description: set number of bangs and start the loop
  - type: bang
    description: start the loop
  2nd:
  - type: float
    description: number of bangs to output

outlets:
  1st:
  - type: bang
    description: bang according to the counter loop
  2nd:
  - type: bang
    description: bangs after the last bang in the loop has fired
  3rd:
  - type: float
    description: bang counter

methods:
  - type: pause/break
    description: stop banging/counting
    default: 
  - type: resume/continue
    description: resume banging/counting
    default: 
  - type: offset <float>
    description: set counter starting point
    default: 

draft: true
---

[uzi] loops firing a given number of bangs (left outlet) and outputs a count for each bang (right outlet). The middle outlet bangs after the loop is done. The 1st argument is the number of bangs. The counter starts from a given offset (2nd argument).
