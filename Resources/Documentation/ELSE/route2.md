---
title: route2

description: [route] variant

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: float/symbol
  description: list of addresses to route to
  default: 

inlets:
  1st:
  - type: anything
    description: any message to be routed according to its 1st element

outlets:
 nth:
  - type: list
    description:  rest of message that corresponds to an argument
  2nd:
    - type: list
      description: input message if no match was found

draft: false
---

[route2] is similar to route, but it doesn't trim the list selector and always outputs messages with a proper selector. Another difference is that you can mix floats and symbols as arguments. Each argument creates a corresponding outlet and there's an extra rightmost outlet for messages that don't match the arguments.
