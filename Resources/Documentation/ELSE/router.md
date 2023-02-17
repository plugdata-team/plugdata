---
title: router

description: route messages

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: number of outlets (2 to 512)
  default: 2
- type: float
  description: sets initially open outlet
  default: 0

inlets:
  1st:
  - type: anything
    description: message to send through a specified outlet
  2nd:
  - type: float
    description: sets outlet number (-1 is none)

outlets:
 nth:
  - type: anything
    description:  outlets for routing any received message

draft: false
---

[router] routes a message from the left inlet to an outlet number specified by the float into the right inlet (if the number is out of range, the message is not output).
