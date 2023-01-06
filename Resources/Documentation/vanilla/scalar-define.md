---
title: scalar define
description: defines and maintains a scalar.
categories:
- object
see_also: 
- array
- text
- list
pdcategory: Accessing Data
last_update: '0.49'
inlets:
  1st:
  - type: bang
    description: output a pointer to the scalar.
  - type: send <symbol>
    description: send pointer to a named receive object
outlets:
  1st:
  - type: pointer
    description: a pointer to the scalar.
flags:
- flag: -k
  description: saves/keeps the contents with the patch.
arguments:
- type: symbol
  description: template name.
draft: false
---
create, store, and/or edit one

`[read scalar-object-help.txt(` read/write a file (TBW)

`[send scalar-help-send(` send a pointer to a named receive object