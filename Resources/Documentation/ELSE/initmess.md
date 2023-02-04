---
title: initmess
description: initialization message

categories:
 - object

pdcategory: ELSE, Data Management, Triggers and Clocks

arguments:
- type: anything
  description: the message, where commas and semicolons behave as usual in message boxes, dollar signs behave as usual inside objects too
  default: none

inlets:
  1st:
  - type: bang
    description: resends message

outlets:
  1st:
  - type: anything
    description: the message

---

The [initmess] object sends messages when loading the patch. As an object, it can also deal with "$0" and arguments and expand them.

