---
title: message

description: store messages

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: anything
  description: message output
  default: bang

inlets:
  1st:
  - type: bang
    description: outputs the message
  - type: anything
    description: sets the message

  2nd:
  - type: anything
    description: sets the message

outlets:
  1st:
  - type: anything
    description: message output

draft: false
---

The [message] object stores and outputs any kind of message in Pd. As an object, $1/$2 loads arguments when used in an abstraction, so the syntax is not the same as a message box. Nonetheless, it deals with comma and semicolons in the same way as regular message boxes do. See [pd syntax] for more details.
