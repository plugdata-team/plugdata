---
title: initmess
description: initialization message

categories:
 - object

pdcategory: ELSE, Data Management, Triggers and Clocks

arguments:
- type: anything
  description: commas and semicolons behave as in message boxes, $ behave as in objects
  default: none

inlets:
  1st:
  - type: bang
    description: resends message

outlets:
  1st:
  - type: anything
    description: the message

draft: false
---

The [initmess] object sends messages when loading the patch. As an object, it can also deal with "$0" and arguments and expand them.

