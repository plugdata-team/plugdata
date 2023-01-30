---
title: nmess

description: message gate

categories:
- object

pdcategory: ELSE, Data Management

arguments:
- description: sets 'n' number of messages
  type: float
  default: 0

inlets:
  1st:
  - type: anything
    description: a message to be gated
  2nd:
  - type: bang
    description: resets and reopens gate
  - type: float
    description: resets and sets 'n' number of messages

outlets:
  1st:
  - type: anything
    description: output a message if the gate is opened
  2nd:
  - type: anything
    description: output a message if the gate is closed

draft: false
---

[nmess] allows 'n' number of messages to be gated through the left outlet (it goes to the right outlet otherwise). A bang in the right inlet resets and reopens the gate.
