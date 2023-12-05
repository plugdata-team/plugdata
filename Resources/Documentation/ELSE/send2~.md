---
title: send2~

description: multichannel signal send

categories:
 - object
pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: send symbol 
  default: none

inlets:
  1st:
  - description: signals to send
    type: signals

draft: false
---

[send2~] is like send but automatically resizes according to the number of input channels.
