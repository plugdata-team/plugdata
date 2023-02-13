---
title: scalar
description: create a scalar datum 
  default: [scalar define]
categories:
- object
see_also: 
- array
- text
- list
pdcategory: vanilla, Data Structures
last_update: '0.49'
inlets:
  1st:
  - type: bang
    description: output a pointer to the scalar
outlets:
  1st:
  - type: pointer
    description: a pointer to the scalar
flags:
- name: -k
  description: saves/keeps the contents with the patch
arguments:
- type: symbol
  description: template name
methods:
  - type: send <symbol>
    description: send pointer to a named receive object
draft: false
---
experimental - doesn't do much yet. This has been included in 0.45 to check that its design will work coherently with the array and text objects

