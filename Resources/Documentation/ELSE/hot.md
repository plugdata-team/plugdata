---
title: hot
description: make all inputs hot

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: sets 'n' number of inlets/outlets (max 128)
  default: 2

inlets:
  nth:
  - type: bang
    description: outputs last stored values
  - type: anything
    description: any messages go to the corresponding outlet

outlets:
  nth:
  - type: anything
    description: messages from corresponding input

methods:
  - type: set <anything>
    description: set any input message without output

draft: false
---

[hot] outputs messages when any inlet gets a message, making them all hot. Output is from right to left.

