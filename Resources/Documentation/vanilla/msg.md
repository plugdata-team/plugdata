---
title: msg
description: Message box
pdcategory: GUI
inlets:
  1st:
  - type: anything
    description: Trigger output
  - type: set <anything>
    description: Sets message

outlets:
  1st:
  - type: anything
    description: Message output
draft: false

methods:
- type: add
  description: add message terminating by a semicolon.
- type: add2
  description: add message without the terminating semicolon.
- type: addcomma
  description: add a comma.
- type: addsemi
  description: add a semicolon.
- type: adddollar <float>
  description: add a dollar-sign argument.
- type: adddollsym <symbol>
  description: Add a dollar-sign symbol that starts with a float.