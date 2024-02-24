---
title: sender

description: send multiple messages

categories:
 - object

pdcategory: ELSE, UI, Mixing and Routing

arguments:
- type: float
  description: optional depth value
  default: 0
- type: list
  description: up to two send names
  default: 0

inlets:
  1st:
  - description: messages to send
    type: anything
  2nd:
  - description: send symbol name 1
    type: symbol
  3rd:
  - description: send symbol name 2
    type: symbol

draft: false
---

[sender] is much like vanilla's [send], but can have up to two names and at load time can expand dollar symbols according to parent patches.
