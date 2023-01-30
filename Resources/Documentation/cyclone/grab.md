---
title: grab
description: grabs the output of another object
categories:
 - object
pdcategory: General
arguments:
- type: float
  description: number of outlets besides the rightmost one
  default: 1
- type: symbol
  description: messages in the inlet are sent to receive objects named by this symbol. In this case, there's no rightmost outlet
inlets:
  1st:
  - type: anything
    description: a message to send an object whose output is grabbed
outlets:
  nth:
  - type: anything
    description: anything that was grabbed from another object
  2nd: #rightmost
  - type: anything
    description: the message sent to an object whose output is grabbed

methods:
  - type: set <symbol>
    description: sets the receive name (if a 2nd argument was given)

---

[grab] sends a message to another object and "grabs" its output, sending it through its outlet(s) isntead of the grabbed object.

