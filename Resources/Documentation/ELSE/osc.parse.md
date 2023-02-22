---
title: osc.parse
description: parse OSC messages

categories:
 - object

pdcategory: ELSE, Networking

arguments:

inlets:
  1st:
  - type: list
    description: formatted OSC message

outlets:
  1st:
  - type: anything
    description: parsed OSC message
  2nd:
  - type: float
    description: timetag offset in milliseconds
draft: false
---

[osc.parse] is similar to Vanilla's [oscparse] but the output is not a list and more closely related on how OSC messages are generally dealt with. It is still in the [osc.receive] abstraction and you can use the object for more edge and lower level cases.

