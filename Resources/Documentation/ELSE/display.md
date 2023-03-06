---
title: display

description: display messages

categories:
- object

pdcategory: ELSE, UI

arguments:
- description: number of characters
  type: float
  default: 0 - not fixed
- description: refresh rate in ms
  type: float
  default: 100

inlets:
  1st:
  - type: anything
    description: any message to be displayed

outlets:
  1st:
  - type: anything
    description: any input is passed through

draft: false
---

[display] can display a message just like [print]. The 1st argument specifies a fixed number of characters to display. If "0" or no argument is given, the display automatically resizes according to the input message. If a message is larger than the fixed number of characters, the displayed message is truncated.

