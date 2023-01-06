---
title: initmess
description: Initialization message

categories:
 - object

pdcategory: General

arguments:
- type: anything
  description: the message, where commas and semicolons behave as usual in message boxes, dollarsigns behave as usual inside objects too
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

