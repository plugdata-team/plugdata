---
title: daw_storage

description: store data inside DAW session state

categories:
 - object

pdcategory: ELSE, Signal Math

arguments:
- type: symbol
  description: unique ID for DAW storage data
  default: empty

inlets:
  1st:
  - type: anything
    description: extra data to be stored

outlets:
  1st:
  - type: anything
    description: last data on patch load


draft: false
---
[daw_storage] allows storage of arrays, lists, symbols, as extradata lists in the DAW session.
