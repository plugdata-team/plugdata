---
title: group

description: group messages into a list

categories:
- object

pdcategory: ELSE, Data Management

arguments:
- description: group size
  type: float
  default: 0

inlets:
  1st:
  - type: anything
    description: input messages to be regrouped
  - type: bang
    description: outputs the remainder
  2nd:
  - type: float
    description: calculate or output the last calculated value

outlets:
  1st:
  - type: anything
    description: the regrouped message
  2nd:
  - type: bang
    description: when there's no remainder (group is empty)

flags:
  - name: -trim
    description: trims selectors (list/symbol) on the output

methods:
  - type: clear
    description: clears the remainder


draft: false
---

[group] groups messages according to a group size. When the input is smaller than the group size, it needs to reach the group size before it's sent out. If the input list is bigger than the group size, the remainder gets grouped in further lists.

