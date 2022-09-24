---
title: canvas.name

description: Get canvase name

categories:
 - object

pdcategory: Subpatch Management

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
- name: -w
  description: for 'window' name
- name: -env
  description: sets depth to "environment" mode

draft: false
---

[canvas.name] gives you the canvas/window name symbol. See the help file of else/gui that shows how to manipulate canvas widgets with this.