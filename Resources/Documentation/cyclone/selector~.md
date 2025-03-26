---
title: selector~

description: select messages from an inlet

categories:
 - object

pdcategory: cyclone, Mixing and Routing, Data Management

arguments:
- type: float
  description: set the 'n' number of channels (2-512)
  default: 2
- type: float
  description: inlet initially selected
  default: 0

inlets:
  1st:
  - type: float/signal
    description: sets which inlet is selected
  nth:
  - type: signal
    description: any message to be selected

outlets:
  1st:
  - type: signal
    description: message from the selected inlet

draft: true
---

[selector] outputs data from the selected inlet according to the float into the rightmost inlet.
