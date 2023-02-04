---
title: unjoin

description: break a list into separate messages

categories:
 - object

pdcategory: cyclone, Data Management

arguments:
- type: float
  description: number of group outputs
  default: 2

inlets:
  1st:
  - type: anything
    description: any message whose elements will be separated into groups of elements

outlets:
  nth:
  - type: anything
    description: the list composed of the joined messages

- flags:
  - name: @outsize <float>
    description: number of elements per group
    default: 1

methods: 
  - type: outsize <float>
    description: number of elements per group
    default: 1

draft: true
---

[unjoin] separates a list's elements by groups of any size (default 1). Each group is sent out a separate outlet, extra elements are sent to an extra outlet.
