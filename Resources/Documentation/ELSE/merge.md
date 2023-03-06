---
title: merge

description: merge messages

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: float
  description: number of inlets
  default: 2

inlets:
  nth:
  - type: anything
    description: any message type to merge
  - type: bang
    description: outputs last composed message
 
outlets:
  1st:
  - type: list
    description: message composed of the merged messages

flags:
 - name: -trim
   description: trims list selector

draft: false
---

[merge] takes any type of messages in any inlet and combines them into a message.
