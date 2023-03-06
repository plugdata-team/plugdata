---
title: bag
description: collection of numbers
categories:
- object
pdcategory: vanilla, Data Management
last_update: '0.33'
see_also:
- poly
- makenote
inlets:
  1st:
  - type: float
    description: value to store or delete depending on the flag
  2nd:
  - type: float
    description: flag (true <non-0>, or false <0>)
outlets:
  1st:
  - type: float
    description: the stored values on flush message
methods:
  - type: flush
    description: output stored values and clear the bag
  - type: clear
    description: clear stored values from the bag (no output)
draft: false
---
The bag object adds a value to or removes it from a collection of numbers depending on the flag. The left inlet takes the value and the right inlet takes the flag. If the flag is true (non-0), the value is added to the collection and removed otherwise. The example here takes a list input, which gets spread at inlets (as is common in Pd).

The collection may have many copies of the same value. You can output the collection (and empty it) with a "flush" message, or just empty it with "clear." You can use this to mimic a sustain pedal, for example.
