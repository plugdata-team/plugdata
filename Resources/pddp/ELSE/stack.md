---
title: stack

description: Stack messages and output via fifo/lifo

categories:
- object

pdcategory: Message Management 

arguments:
- type: symbol
  description: 1) symbol - sets order, <fifo> or <lifo>
  default: fifo

inlets:
  1st:
  - type: bang
    description: outputs and removes a message
  - type: dump
    description: outputs all messages and clears them
  - type: clear
    description: clears the stack
  - type: n
    description: outputs number of elements in right outlet
  - type: fifo
    description: set order to 'fifo'
  - type: lifo
    description: set order to 'lifo'
  - type: show
    description: opens [text] window
  - type: hide
    description: closes [text] window
  2nd:
  - type: anything
    description: input messages to be stacked

outlets:
  1st:
  - type: anything
    description: the stored message
  2nd:
  - type: float
    description: number of stacked messages

draft: false
---

[stack] stores messages as stacks, where it outputs and removes them one by one when receiving bangs. An argument defines the output order, which can be 'fifo' (first in first out) or 'lifo' (last in first out) - This needs to be set before storing data.
