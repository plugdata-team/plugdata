---
title: default

description: default message

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: anything
  description: initially stored message
  default: "bang"

inlets:
  1st:
  - type: bang
    description: outputs the stored message
  - type: anything
    description: outputs the message without storing it
  2nd:
  - type: anything
    description: stores the message

outlets:
  1st:
  - type: anything
    description: the stored message

draft: false
---

The [default] sets a default message as the argument or via the right inlet. A bang outputs the default message, other messages are sent through without being stored.

