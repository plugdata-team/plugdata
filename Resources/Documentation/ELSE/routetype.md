---
title: routetype

description: route message types

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: anything
  description: list of types to route; no args — everything is sent to the outlet
  default:

inlets:
  1st:
  - type: anything
    description: any message to be routed according to its type

outlets:
  nth:
  - type: anything
    description: any message that corresponds to a type
  2nd:
  - type: anything
    description: non-corresponding messages if less than the 6 types are given

draft: false
---

[routetype] routes messages according to its type, such as "bang", "float", "list" and "symbol". There is also the "anything" type, which is can be defined as "any other" message that is not the former ones.
These 5 types are given as arguments, and there's an outlet corresponding to each given argument that sends out the message type according to the arguments(bang", "float", "list", "symbol" or "any").
