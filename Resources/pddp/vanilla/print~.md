---
title: print~
description: print out raw values of a signal
categories:
- object
see_also:
- print
- block~
pdcategory: General Audio Manipulation
last_update: '0.33'
inlets:
  1st:
  - type: signal
    description: signal block to print on terminal window.
  - type: bang
    description: print one block on terminal window.
  - type: float
    description: sets and prints number of blocks on terminal window. 
arguments:
- type: symbol
  description: symbol to distinct one [print~] from another.
draft: false
---
The print~ object takes a signal input and prints one or more blocks (or 'vectors') out when you send it a bang or a number. By default a block is 64 samples.