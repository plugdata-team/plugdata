---
title: unite

description: unite messages into a symbol

categories:
- object

pdcategory: ELSE, Data Management

arguments:
  - type: symbol
    description: separator (default space)

inlets:
  1st:
  - type: anything
    description: any message to be united/converted into a symbol
  2nd:
  - type: symbol
    description: set separator

outlets:
  1st:
  - type: symbol
    description: the converted symbol message

draft: false
---

[unite] unites and converts messages into a symbol message.
