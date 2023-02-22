---
title: order

description: split and index list elements

categories:
- object

pdcategory: ELSE, Data Management

arguments:
  - description: number of elements to be ordered
    type: float
    default: 1
  - description: index offset
    type: float
    default: 0

inlets:
  1st:
  - type: anything
    description: any number of elements to be split and ordered

outlets:
  1st:
  - type: list
    description: ordered lists, with the split elements preceded by index

methods:
  - type: n <float>
    description: number of elements in each ordered list
  - type: offset <float>
    description: index offset

draft: false
---

[order] splits a list into successive lists of a given size (default 1 element) and tags each of them with ordered indexes. This is useful for sending values to all instances of an object loaded in [clone]!
