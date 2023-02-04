---
title: fromsymbol
description: convert symbol to anything
categories:
 - object
pdcategory: cyclone, Converters
arguments:
inlets:
  1st:
  - type: symbol
    description: the symbol to be converted to any message type
  - type: bang
    description: outputs a bang
  - type: float
    description: just goes right through as a float
  - type: list
    description: if the 1st element is a float, it goes through
  - type: anything
    description: 1st element is treated as a symbol and converted
outlets:
  1st:
  - type: anything
    description: a converted message from a symbol

flags:
  - name: @separator <symbol>
    description: sets the separator character (default "space")

---

[fromsymbol] converts a symbol message to anything (any kind of message). Start by typing a float in the atom symbol box below and see how it converts to a float message, then check more examples to the right.

