---
title: args

description: manage arguments

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
  - type: symbol
    description: (optional) break character
    default: none
  - type: float
    description: depth level
    default: 0

inlets:
  1st:
  - type: bang
    description: output arguments list
  - type: anything
    description: sets new arguments

outlets:
  1st:
  - type: anything
    description: symbol, float or list, depending on the given arguments - or bang if no arguments are given to the parent patch (as in the help patch)

draft: false
---

[args] loads arguments of an abstraction and can also change them. This is useful for the management of a variable number of arguments in abstractions, and can be used to change/set them up.
