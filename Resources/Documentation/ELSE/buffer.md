---
title: buffer

description: get/set array buffer

categories:
 - object

pdcategory: ELSE, Arrays and Tables, Buffers

arguments:
- type: symbol
  description: array name
  default: none

inlets:
  1st:
  - type: list
    description: resize, set array and output list
  - type: bang
    description: output array
  2nd:
  - type: list
    description: resize and set array

outlets:
  1st:
  - type: list
    description:  array values


methods:
  - type: set <list>
    description: 1st item is index, the next are values from that index
  - type: name <symbol>
    description: set array name

draft: false
---

[buffer] does not much and nothing that [array] can't, but it's a convenient object for getting and setting array values, kinda like a storage object such as [message]. You can get/set arrays from actual arrays, [table] or [array] objects.
