---
title: sprintf

description: message formatter

categories:
 - object

pdcategory: cyclone, Data Management

arguments:
- type: symbol
  description: the optional 'symout' argument formats a symbol message
  default:
- type: anything
  description: output message with format types '%' 
  default:

inlets:
  1st:
  - type: bang
    description: formats the message with current input
  nth:
  - type: anything
    description: float/symbol to match corresponding given type

outlets:
  1st:
  - type: anything
    description: the formatted message

draft: true
---

Based on C's "printf" function, [sprintf] formats messages, where each '%' format type corresponds to an inlet.
