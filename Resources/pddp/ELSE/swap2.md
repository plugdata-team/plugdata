---
title: swap2

description: Swap two messages

categories:
- object

pdcategory: Message Management 

arguments: (none)

inlets:
  1st:
  - type: anything
    description: any message to be swapped to the right outlet
  2nd:
  - type: anything
    description: any message to be swapped to the left outlet

outlets:
  1st:
  - type: anything
    description: swapped messages
  2st:
  - type: anything
    description: swapped massages

draft: false
---

[swap2] is like [swap] but swaps any messages, not just floats.
