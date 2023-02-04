---
title: insert

description: insert message in a list

categories:
- object

pdcategory: ELSE, Data Management

arguments:
- description: (optional) index
  type: float
  default: 0
- description: message to insert
  type: float
  default: none

inlets:
  1st:
  - type: bang
    description: outputs last message
  - type: anything
    description: message to have elements inserted into
  2nd:
  - type: anything
    description: elements to insert int a message
  3rd:
  - type: float
    description: index position to insert

outlets:
  1st:
  - type: anything
    description: output the resulting message

draft: false
---

[insert] inserts a message into a list according to an index. An index 0 will prepend the message. The object doesn't output a list selector, use [list] afterwards if you want it.

