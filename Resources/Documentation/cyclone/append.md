---
title: append, cyclone/append

description: append a message to the incoming message

categories:
- object

pdcategory: cyclone, Data Management

arguments:
- description: sets message to append to the input
  type: anything
  default: "nothing"

inlets:
  1st:
  - type: anything
    description: a message that will be combined to the stored message

outlets:
  1st:
  - type: anything
    description: the combined message

methods:
  - type: set <anything>
    description: updates argument (message to append to the input message)

draft: false
---

[append] will add messages set as argument to the end of any message sent to the input.

