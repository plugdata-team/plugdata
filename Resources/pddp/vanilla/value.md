---
title: value, v
description: shared numeric value
categories:
- object
pdcategory: General
last_update: '0.51'
see_also:
- send
- int
- float
- expr
arguments:
- description: sets value name (if no name is given,  a right inlet is created to
    set the name).
  type: symbol
inlets:
  1st:
  - type: bang
    description: outputs the value.
  - type: float
    description: sets variable value.
  - type: send <symbol>
    description: sends the value to a matching receive name.
  '2nd: (if created without argument)':
  - type: symbol
    description: sets the value name.
outlets:
  1st:
  - type: float
    description: sets variable value.
draft: false
aliases:
- v
---
"Value" stores a numeric value which is shared between all values with the same name (which need not be in the same Pd window.)

The value may also be stored or recalled in expressions via the expr, expr~, and fexpr~ objects.

The value object can also receive float values sent via a [send] object or a message if it has a variable with the same name.
