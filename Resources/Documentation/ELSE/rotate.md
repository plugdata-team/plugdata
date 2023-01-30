---
title: rotate

description: rotate messages

categories:
- object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: the rotation number
  default:

inlets:
  1st:
  - type: anything
    description: the incoming message/list
  2nd:
  - type: float
    description: the rotation number

outlets:
  1st:
  - type: anything
    description: the rotated message/list

draft: false
---

[rotate] rotates messages/lists. A rotation number sets the offset. If it's positive, the list is shifted to the right. If negative, to the left.
