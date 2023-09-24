---
title: clone
description: make multiple copies of an abstraction
categories:
- object
see_also: 
- poly
pdcategory: vanilla, UI, Data Management
last_update: '0.54'
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
    description: output from all instances either as multichannel signal or summed into mono signal

flags:
  - name: -x
    description: avoids including a first argument setting voice number
  - name: -s <float>
    description: sets starting voice number
    default: 0
  - name: -di
    description: distribute multichannel input signals across cloned patches
  - name: -do
    description: combine signal outputs to make a multichannel signal
  - name: -d
    description: set both -di and -do flags

arguments:
  - type: symbol
    description: abstraction name
  - type: float
    description: number of instances
  - type: list
    description: optional arguments to the abstraction

methods:
  - type: resize <float>
    description: resizes the number of copies
  - type: vis <list>
    description: opens a copy, takes copy number and visualization status (1 to open, 0 to close)
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
