---
title: prepend

description: prepend a message to the incoming message

categories:
 - object

pdcategory: cyclone, Data Management

arguments:
- type: anything
  description: sets message to append to input
  default: "nothing"

inlets:
  1st:
  - type: anything
    description: a message that will be combined to the stored message

outlets:
  1st:
  - type: anything
    description:  the combined message

methods:
- type: set <anything>
  description: updates argument (message to prepend to the input message)

draft: true
---

[prepend] will add messages set as argument to the beginning of any message sent to the input.
