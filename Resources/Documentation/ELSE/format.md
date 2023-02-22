---
title: format
description: format messages

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: anything
  description: atoms that may contain '%' variables (obligatory)

inlets:
  nth:
  - type: bang
    description: outputs the formatted message
  - type: anything
    description: float/symbol atoms to format variables (messages with more than one item and sends the remaining items to the next inlets)

outlets:
  1st:
  - type: anything
    description: the formatted message

draft: false
---

[format] formats messages similarly to [makefilename], but it accepts more than one variable where each corresponds to an inlet. It also allows you to generate messages with more than on element.

