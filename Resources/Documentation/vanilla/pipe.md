---
title: pipe
description: dynamically growable delay line for numbers
categories:
- object
pdcategory: vanilla, Triggers and Clocks
last_update: '0.33'
see_also:
- delay
- timer
arguments:
- description: (optional) symbols sets number of inlets and type (f, s, p) and floats set float type and initial value
  type: list
  default: f
- description: sets delay time in ms 
  default: 0
  order: last
  type: float
inlets:
  1st:
  - type: bang
    description: sends the last received data after the delay time
  - type: float/symbol/pointer
    description: the type depends on the creation argument
  nth:
  - type: float/symbol/pointer
    description: the type depends on the creation argument
  2nd: #rightmost
  - type: float
    description: set the delay time in ms
outlets:
  nth:
  - type: float/symbol/pointer
    description: the type depends on the creation argument
methods:
  - type: clear
    description: forget all scheduled messages
  - type: flush
    description: sends the scheduled messages immediately
draft: true #this will display the wrong second inlet description when there's only 2 inlets
---
Message "delay line"

The [pipe] object stores a sequence of messages and outputs them after a specified delay time in milliseconds. The output is scheduled when storing the incoming message. Thus changing the delay time doesn't affect the messages that are already scheduled.

You can specify the data type with a first argument (which is a float by default
