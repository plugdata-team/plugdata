---
title: morph

description: morph between lists

categories:
- object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: morph time in ms
  default: 0
- type: float
  description: grain time in ms
  default: 20
- type: list
  description: start list values
  default: 0

inlets:
  1st:
  - type: list
    description: list with one or more elements to morph to
  2nd:
  - type: float
    description: morph time in ms
  3rd:
  - type: float
    description: grain time in ms

outlets:
  1st:
  - type: list
    description: morphed list

flags:
  - name: -exp <float>
    description: sets exponential factor 

methods:
  - type: set <list>
    description: set start list values

draft: false
---

[morph] is an abstraction based on vanilla's [line], but besides floats, it also works for lists - so you can transition (or morph) between them. It takes a time to reach the target and a grain time just like [line].
