---
title: canvas.name

description: get canvas name

categories:
 - object

pdcategory: ELSE, UI

arguments:
- type: float
  description: depth level
  default: 0

inlets:
  1st:
  - type: bang
    description: outputs canvas name

outlets:
  1st:
  - type: symbol
    description: canvas name

flags:
- name: -env
  description: sets depth to "environment" mode

draft: false
---

[canvas.name] gives you the canvas/window name symbol.
