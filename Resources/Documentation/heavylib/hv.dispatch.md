---
title: hv.dispatch

description: automatically generate local send object for parameter distribution

categories:
- object

pdcategory: heavylib, GUI, Mixing and Routing

arguments:
- type: symbol
  description: unique ID
- type: symbol
  description: parameter name
- type: float
  description: default value

inlets:
  1st:
  - type: anything
    description: parameter name followed by message to send

outlets:
  1st:
  - type: anything
    description: input with matching parameter trimmed

draft: false
---
hv.dispatch creates local send objects. it's a way to funnel messages into a single abstraction input and then easily be able to distribute them. it outputs the remainder of messages, so that you can chain them together.
