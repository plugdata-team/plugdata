---
title: stream

description: pack a stream of numbers

categories:
- object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: N group size
  default: 1

inlets:
  1st:
  - type: float
    description: input stream of numbers
  - type: bang
    description: resends the last output list
  2nd:
  - type: float
    description: N group size

outlets:
  1st:
  - type: anything
    description: the regrouped message

methods:
  - type: clear
    description: clears the list

draft: false
---

[stream] mode makes a list with the last N received items. The N is set as an argument or in the right inlet. A negative N inverts the list.
