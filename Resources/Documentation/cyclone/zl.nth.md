---
title: zl.nth

description: list processor

categories:
 - object

pdcategory: cyclone, Data Management

arguments:
- type: float
  description: maximum list size (optional, 1 - 32767)
  default: 256
- type: symbol
  description: mode
  default:

inlets:
  1st:
  - type: bang
    description: mode dependent operation on current data see
  - type: anything
    description: one or more element messages to be processed
  2nd:
  - type: anything 
    description: depends on the mode, see [pd examples]

outlets:
  1st:
  - type: anything
    description: output according to the mode: see details in [pd examples]
  2nd:
  - type: anything
    description: output according to the mode: see details in [pd examples]

flags:
  - name: @zlmaxsize <float>
    description: max list size (1 - 32767)
    default: 256

methods:
  - type: mode <symbol>
    description: sets the mode (change, compare, delace, ecils, group, indexmap, iter, join, lace, len, lookup, median, mth, nth, queue, reg, rev, rot, sect, scramble, slice, sort, stack, stream, sub, sum, swap, thin, union or unique)
  - type: zlclear
    description: clears mode's arguments and received data in both inlets
  - type: zlmaxsize <float>
    description: sets the maximum list size (1 - 32767, default 256)

draft: true
---

[zl] processes messages with one or more elements ("list messages' or "anything") according to a mode (set via argument/message).
