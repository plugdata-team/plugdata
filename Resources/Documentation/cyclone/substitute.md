---
title: substitute

description: substitute elements in messages

categories:
 - object

pdcategory: cyclone, Data Management

arguments:
- type: float/symbol
  description: the match element that should be replaced 
  default: 0
- type: float/symbol
  description: the replacement for the match 
  default: 0
- type: float/symbol
  description: any float or symbol sets substitute to "first only" mode
  default: 0

inlets:
  1st:
  - type: anything
    description: any message to have elements substituted or not
  2nd:
  - type: anything
    description: sets the 1st element to be substituted by the 2nd

outlets:
  1st:
  - type: anything
    description: the substituted message if a substitution occurred
  2nd:
  - type: anything
    description: the input message if no substitution occurred

methods:
  - type: set <anything>
    description: sets the 1st element to be substituted by the 2nd (other elements are ignored)
    default: 


draft: true
---

[substitute] exchanges an element (the first argument) in an input message to another one (second argument).
