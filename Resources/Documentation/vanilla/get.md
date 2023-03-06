---
title: get
description: get values from a scalar
categories:
- object
see_also: 
- pointer
- set
- append
- getsize
- setsize
- element
- struct
pdcategory: vanilla, Data Structures
last_update: '0.47'
inlets:
  1st:
  - type: pointer
    description: pointer to a data structure scalar
outlets:
  nth:
  - type: float/symbol
    description: field value
arguments:
- type: symbol
  description: template name
- type: list
  description: one or more field names (defines number of outlets)
methods:
  - type: set <symbol, symbol>
    description: set template and field name (if none or just one argument is given)
draft: false
---
"Get", when sent a pointer to a scalar, retrieves fields from it by name. The fields can be floats or symbols.

If you have data whose template is variable (from a heterogeneous list, for example) you can use the template "-" as a wild card. In Pd 0.47 and earlier, there is no penalty for this, but future versions may run faster with pre-specified templates than with "-" (by looking up the variable names in advance).
