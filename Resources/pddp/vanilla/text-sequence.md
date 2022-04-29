---
title: text sequence
description: sequencer/message-sender.
categories:
- object
pdcategory: Misc
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
- description: 'text name if no flags are given (default: none).'
  type: symbol
flags:
- description: struct name and field name of main structure.
  flag: -s <symbol, symbol>
- description: sets to global mode (with symbolic destinations).
  flag: -g
- description: sets symbols that define waiting points.
  flag: -w <symbol>
- description: sets number of leading floats used as waiting points.
  flag: -w <float>
- description: sets tempo value and time unit.
  flag: -t <float, symbol>
inlets:
  1st:
  - type: auto
    description: automatically sequence interpreting waits as delay times.
  - type: stop
    description: stops the sequence when in auto mode.
  - type: step
    description: output next line.
  - type: line <float>
    description: set line number (counting from 0).
  - type: args <list>
    description: set values for $1, $2, etc in the text.
  - type: bang
    description: output all lines from current to next waiting point.
  - type: list
    description: same as bang but temporarily override 'args' with list's elements
      (a bang is a 0 element list, btw).
  - type: tempo <f, sym>
    description: set tempo value (float) and time unit symbol.
  2nd:
  - type: symbol
    description: set text name.
  - type: pointer
    description: pointer to the text if -s flag is used.
outlets:
  1st:
  - type: list
    description: messages from the sequence or waits if -g flag is given.
  2nd:
  - type: list
    description: waits if -w flag is given (which creates a mid outlet).
  rightmost:
  - type: bang
    description: when finishing the sequence.
draft: false
---
"text sequence" outputs lines from a text buffer, either to an outlet or as messages to named destinations. The text is interpreted as a sequence of lists, and possibly some interspersed waiting instructions (called "waits" here). You can ask for one line at a time ("step" message), or to proceed without delay to the next wait ("bang"), or to automatically sequence through a series of waits (with each wait specifying a delay in milliseconds).
