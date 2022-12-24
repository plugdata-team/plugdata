---
title: routetype

description: Route message types

categories:
 - object

pdcategory: Message management

arguments:
- type: anything
  description: list of message types to route (bang/b, float/f, symbol/s, list/l, anything/a, pointer/p). If no argument is given, all message types are sent to the outlet
  default:

inlets:
  1st:
  - type: anything
    description: any message to be routed according to its type

outlets:
  nth:
  - type: anything
    description: any message that corresponds to a type defined by the argument
  2nd:
  - type: anything
    description: if less than the 6 types are given as arguments, an extra outlet sends the uncorresponding messages

draft: false
---

[routetype] routes messages according to its type, such as "bang", "float", "list" and "symbol". There is also the "anything" type, which is can be defined as "any other" message that is not the former ones.
These 5 types are given as arguments, and there's an outlet corresponding to each given argument that sends out the message type according to the argumentm(bang", "float", "list", "symbol" or "any").