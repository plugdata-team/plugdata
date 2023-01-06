---
title: trigger, t
description: sequence messages in right-to-left order.
categories:
- object
pdcategory: General
last_update: '0.52'
see_also:
- bang
- unpack
arguments:
- description: 'symbols that define outlet''s message type: float'',  ''bang'',  ''symbol'',  ''list'',  ''anything''
    and ''pointer'',  all of which can be abreviatted 
  default:: f f
.'
  type: list
inlets:
  1st:
  - type: anything
    description: any message to be sequenced over the outlets.
outlets:
  'n: (depends on the number of arguments