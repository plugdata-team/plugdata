---
title: text sequence
description: sequencer/message-sender
categories:
- object
pdcategory: vanilla, Data Management, Triggers and Clocks
last_update: '0.49'
see_also:
- list
- array
- scalar
- text define
- text get
- text set
- text insert
- text delete
- text size
- text tolist
- text fromlist
- text search
- text
arguments:
- description: array name if no flags are given 
  default: none
  type: symbol
flags:
- name: -s <symbol, symbol>
  description: struct name and field name of main structure
- name: -g
  description: sets to global mode (with symbolic destinations)
- name: -w <symbol>
  description: sets symbols that define waiting points
- name: -w <float>
  description: sets number of leading floats used as waiting points
- name: -t <float, symbol>
  description: sets tempo value and time unit
inlets:
1st:
- type: bang
  description: output all messages from current to next waiting point
- type: list
  description: same as bang but temporarily override 'args' with list's elements
2nd:
- type: symbol
  description: set array name
- type: pointer
  description: pointer to the array if -s flag is used
outlets:
1st:
- type: list
  description: messages from the sequence or waits if -g flag is given
2nd:
- type: list
  description: waits if -w flag is given (which creates a mid outlet)
3rd: #needs to be rightmost if there's one or two other outlets
- type: bang
  description: when finishing the sequence

methods:
- type: auto
  description: automatically sequence interpreting waits as delay times
- type: stop
  description: stops the sequence when in auto mode
- type: step
  description: output next line
- type: line <float>
  description: set line number (actually "message number", from 0)
- type: args <list>
  description: set values for $1, $2, etc in the text
- type: tempo <f, sym>
  description: set tempo value (float) and time unit symbol

draft: true
---
