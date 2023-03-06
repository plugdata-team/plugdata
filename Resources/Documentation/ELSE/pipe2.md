---
title: pipe2

description: delay messages

categories:
- object

pdcategory: ELSE, Data Management

arguments:
- description: delay time in ms
  type: float
  default: 0

inlets:
  1st:
  - type: anything
    description: message to be delayed
  2nd:
  - type: float
    description: sets delay time in ms

outlets:
  1st:
  - type: anything
    description: the delayed message

methods:
  - type: clear
    description: clears the delay time
  - type: flush
    description: outputs all stored elements

draft: false
---

[pipe2] is similar to vanilla's pipe. It takes all kinds of messages and delays them.

