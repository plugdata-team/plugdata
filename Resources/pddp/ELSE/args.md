---
title: args

description: Manage arguments

categories:
 - object

pdcategory: General

arguments:
  1st:
  - type: symbol
    description: (optional) break character
    default: none
  2nd:
  - type: float
    description: depth level
    default: 0

inlets:
  1st:
  - type: bang
    description: output arguments list
  2nd:
  - type: anything
    description: sets new arguments

outlets:
  1st:
  - type: anything
    description: symbol, float or list, depending on the given arguments - or bang if no arguments are given to the parent patch (as in the help patch)

draft: false
---

[args] loads arguments of an abstraction and can also change them. This is useful for the management of a variable number of arguments in abstractions, and can be used to change/set them up.
