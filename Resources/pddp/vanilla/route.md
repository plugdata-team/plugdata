---
title: route
description: route messages according to first element
categories:
- object
pdcategory: General
last_update: '0.43'
see_also:
- select
arguments:
- description: of floats or symbols to route to to 
  default: 0
  type: list
inlets:
  1st:
  - type: anything
    description: any message to route according to the first element.
  2nd:
  - type: float/symbol
    description: if there's one argument,  an inlet is created to update it.
outlets:
  nth:
  - type: anything
    description: routed message with the first element trimmed
  2nd: #this does work to choose the rightmost iolet, but should we use it?
  - type: anything
    description: when input doesn't match the arguments it's passed here

