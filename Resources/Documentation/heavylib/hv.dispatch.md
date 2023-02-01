---
title: hv.dispatch

description: automatically generate local send object for parameter distribution

categories:
- object

pdcategory: heavylib, GUI, Data Management

arguments:
- type: symbol
  description: unique ID
  default: 
- type: symbol
  description: parameter name
  default: 
- type: float
  description: default value
  default: 

inlets:
  1st:
  - type: anything
    description: input to be sent

outlets:
  1st:
  - type: anything
    description: (?)

draft: true
---

