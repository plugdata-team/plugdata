---
title: clone
description: make multiple copies of an abstraction
categories:
- object
see_also: 
- poly
pdcategory: vanilla, UI, Data Management
last_update: '0.47'
inlets:
  nth:
  - type: list
    description: first number selects instance; rest of the list is sent to that instance's inlet
  - type: signal
    description: signal sent to all instances
outlets:
  nth:
  - type: data outlets
    description: output with a prepended instance number
  - type: signal outlets
    description: output the sum of all instances' outputs

flags:
  - name: -x
    description: avoids including a first argument setting voice number
  - name: -s <float>
    description: sets starting voice number (default: 0)

arguments:
  - type: symbol
    description: abstraction name
  - type: float
    description: number of instances
  - type: list
    description: optional arguments to the abstraction

methods:
  - type: next <list>
    description: forwards a message to the next instance's inlet (incrementing and repeating circularly)
  - type: this <list>
    description: forwards a message to the previous instance's inlet sent to by "this" or "next"
  - type: set <list>
    description: sets the "next"/"this" counter
  - type: all <list>
    description: sends a message to all instances' inlet
draft: false
---
clone creates any number of instances of a desired abstraction (a patch loaded as an object in another patch
