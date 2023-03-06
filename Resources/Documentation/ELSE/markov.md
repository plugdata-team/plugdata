---
title: markov

description: create/play Markov chains

categories:
- object

pdcategory: ELSE, Data Management, Data Math, Triggers and Clocks

arguments:
- type: float 
  description: sets order
  default: 1
- type: float
  description: non-0 sets to savestate mode 
  default: 0

inlets:
  1st:
  - type: bang
    description: get value from Markov chain and goes to the next

outlets:
  1st:
  - type: anything
    description: output from Markov chain

flags:
  - name: seed <float>
    description: sets seed

methods:
  - type: create
    description: create Markov chain from memory and go to start
  - type: clear
    description: clears memory
  - type: learn <anything>
    description: feed memory with new information
  - type: savesate <float>
    description: 
  - type: reset
    description: go to the start of the chain
  - type: random
    description: go to a random position in the chain
  - type: order <float>
    description: sets order
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal

draft: false
---

[markov] creates Markov chains of any order out of progressions of floats, symbols or lists (which can be used to create polyphonic chains). You can change the order and recreate the chain. You can keep feeding information after the chain was created and recreate with the new information. You can also clear the memory to restart from scratch. State saving is possible with the 2nd argument, where the chain is saved with the patch.
