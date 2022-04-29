---
title: clone
description: make multiple copies of an abstraction.
categories:
- object
see_also: 
- poly
pdcategory: Subwindows
last_update: '0.47'
inlets:
  'n: (number and type depends on the abstraction)':
  - type: list
    description: first number sets the copy number and the rest of the list is sent to that instance's inlet.
  - type: signal
    description: for signal inlets, audio is sent to all copies.
  - type: next <list>
    description: forwards a message to the next instance's inlet (incrementing and repeating circularly).
  - type: this <list>
    description: forwards a message to the previous instance's inlet sent to by "this" or "next".
  - type: set <list>
    description: sets the "next"/"this" counter.
  - type: all <list>
    description: sends a message to all instances' inlet.
outlets:
  'n (depends on the abstraction)':
  - type: control outlets
    description: output the message with a prepended copy number.
  - type: signal outlets
    description: output the sum of all instances' outputs.
flags:
  - flag: -x
    description: avoids including a first argument setting voice number.
  - flag: -s <float>
    description: sets starting voice number 
  default: 0
.	
arguments:
  - type: symbol
    description: abstraction name.
  - type: float
    description: number of copies.
  - type: list
    description: optional arguments to the abstraction.
draft: false
---
clone creates any number of copies of a desired abstraction (a patch loaded as an object in another patch