---
title: store

description: Store messages sequentially

categories:
- object

pdcategory: Message Management 

arguments:
- type: float
  description: float - non-zero sets to store elements with the patch
  default: 0

inlets:
  1st:
  - type: bang
    description: outputs element from sequence and goes forward
  - type: float
    description: sets index and outputs stored messages at index
  - type: goto <f>
    description: sets index to output on next bang message
  - type: dump
    description: outputs all stored messages sequentially
  - type: clear
    description: clears all messages
  - type: n
    description: outputs number of elements in the right outlet
  - type: show
    description: opens [text] window
  - type: hide
    description: closes [text] window
  2nd:
  - type: anything
    description: messages to be stored in a sequence

outlets:
  1st:
  - type: anything
    description: the stored message
  2nd:
  - type: anything
    description: bang at the end of sequence or 'n' number of messages

click: clicking on the object opens [store]'s window

draft: false
---

[store] is an abstraction based on [text] that stores incoming messages sequentially.
