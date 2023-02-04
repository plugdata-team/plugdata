---
title: canvas.setname

description: set canvas name

categories:
 - object

pdcategory: ELSE, UI

arguments:
- type: symbol
  description: canvas name
  default: none
- type: float
  description: depth level
  default: 0

inlets: none

outlets: none

draft: false
---

[canvas.setname] sets a symbol name to a canvas so you can send it messages. It's the same as [namecanvas] but it also allows you to set the name of a parent patch with the second optional depth argument - (1) is parent patch (2) is parent's parent patch and so on...
