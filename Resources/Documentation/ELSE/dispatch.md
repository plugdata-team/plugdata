---
title: dispatch

description: dispatch elements of a list to different locations

categories:
 - object
pdcategory: ELSE, UI

arguments:
- type: list
  description: receive addresses
  default: none

inlets:
  1st:
  - description: list to dispatch its elements
    type: list
  - description: dispacthes last input again.
    type: bang

draft: false
---

[dispatch] gets a list and sends each element to a given address (order is left to right).
