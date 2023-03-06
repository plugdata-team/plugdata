---
title: scala

description: import 'scala' files

categories:
- object

pdcategory: ELSE, Tuning

arguments:
- type: symbol
  description: file name
  default: none


inlets:
  1st:
  - type: bang
    description: convert/reconvert

outlets:
  1st:
  - type: list
    description: converted scale (in cents)
  2nd:
  - type: anything
    description: file name, scale name

methods:
  - type: symbol
    description: file name

draft: false
---

[scala] imports scales saved in the .scl format and outputs the scale as a list as understood by objects in ELSE. The Scala software is used to create and archive musical scales amongst other things. They provide a database of scales with over 5100 tunings. Check links:
