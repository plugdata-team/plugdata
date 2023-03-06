---
title: value, v
description: shared numeric value
categories:
- object
pdcategory: vanilla, Mixing and Routing, Data Management
last_update: '0.51'
see_also:
- send
- int
- float
- expr
arguments:
- description: sets value name (optional)
  type: symbol
inlets:
  1st:
  - type: bang
    description: outputs the value
  - type: float
    description: sets variable value
  2nd:
  - type: symbol
    description: sets the value name
outlets:
  1st:
  - type: float
    description: sets variable value
methods:
  - type: send <symbol>
    description: sends the value to a matching receive name
draft: false
---
"Value" stores a numeric value which is shared between all values with the same name (which need not be in the same Pd window.)

The value may also be stored or recalled in expressions via the expr, expr~, and fexpr~ objects.

The value object can also receive float values sent via a [send] object or a message if it has a variable with the same name.
