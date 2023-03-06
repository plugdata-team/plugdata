---
title: pointer
description: point to an object belonging to a template
categories:
- object
pdcategory: vanilla, Data Structures
arguments:
- type: list
  description: template names. templates create corresponding outlets plus an extra outlet for non matching templates. If no args are given, 2 outlets are created
inlets:
  1st:
  - type: bang
    description: output the current value
  2nd:
  - type: pointer
    description: store the pointer value (no output)
outlets:
  nth:
  - type: pointer
    description: pointers of matching templates
  2nd:
  - type: pointer
    description: pointers for non matching templates
  3rd: #wrong! should be rightmost even if we only have 2
  - type: bang
    description: when reaching the end of a list

methods:
  - type: traverse <symbol>
    description: sets to the canvas' "head" of the list, the symbol needs to be in the format 'pd-canvasname'
  - type: next
    description: move and output next pointer or "bang" to right outlet if we reach the end of the list
  - type: rewind
    description: goes back to the head of the list and output its pointer (unless the end of list was reached)
  - type: vnext <float>
    description: outputs the next object (if 0) or the next selected object (if 1) or "bang" to right outlet if we reach the end of the list
  - type: delete
    description: delete the current object and output the next (or send a "bang" to the right outlet if it was the last one)
  - type: send <symbol>
    description: send pointer to a receive name given by the symbol
  - type: send-window <any>
    description: send any message to the canvas containing the scalar
  - type: equal <pointer>
    description: compare an incoming pointer with the stored pointer
draft: true
---

