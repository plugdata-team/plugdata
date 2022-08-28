---
title: break

description: Break lists

categories:
 - object

pdcategory: General

arguments:
- type: symbol
  description: separator symbol

inlets:
  1st:
  - type: anything
    description: message to be broken apart

outlets:
  1st:
  - type: anything
    description: broken lists

draft: false
---

[break] can break a list with a given symbol separator set via the argument.
