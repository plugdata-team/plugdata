---
title: float
description: store and recall a number
categories:
- object
pdcategory: General
last_update: '0.48'
see_also:
- int
- value
- send
- symbol
arguments:
- description: initially stored value (default 0).
  type: float
inlets:
  1st:
  - type: bang
    description: output the stored value.
  - type: float
    description: store and output the value.
  - type: list
    description: considers the first element if it's a float,  stores and outputs
      it.
  - type: send <symbol>
    description: send the stored value to a [receive] or [value] object that has the
      same name as the symbol (no output).
  - type: symbol
    description: symbols that look like a float are converted,  stored and output.
  2nd:
  - type: float
    description: store the value (no output).
outlets:
  1st:
  - type: float
    description: the stored value.
aliases:
- f
draft: false
---
Store a (floating point) number

The float object stores a number,  initialized by its creation argument,  which may be reset using its inlet and output by sending it the "bang" message. Sending a number sets a new value and outputs it.
