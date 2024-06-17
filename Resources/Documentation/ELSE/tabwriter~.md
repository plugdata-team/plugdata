---
title: tabwriter~

description: record signals into arrays

categories:
- object

pdcategory: ELSE, Arrays and Tables, Buffers

arguments:
  - type: symbol
    description: array name (optional)
    default:
  - type: float
    description: channels to record (max 64)
    default: 1

flags:
  - name: -continue <f>
    description:
  - name: -loop <f>
    description:
  - name: -start <f>
    description:
  - name: -end <f>
    description:
  - name: -ch <f>
    description: number of channels


inlets:
  1st:
  - type: signal
    description: signal to record into an array channel 0
  - type: rec
    description: (re)starts recording
  - type: stop
    description: stops recording
  nth:
  - type: signal
    description: signal to record into an array channel $nth
  2nd:
  - type: signal
    description: non-0 starts recording at that index, 0 stops it

outlets:
  1st:
  - type: signal
    description: output index we're recording into
  2nd:
  - type: bang
    description: when recording reaches the end of the array

methods:
  - type: set <symbol>
    description: sets array for recording signals
  - type: start <float>
    description: start point in ms (default 0)
  - type: end <float>
    description: end point in ms (default array's initial size)
  - type: range <f, f>
    description: sets start and end point range proportionally (from 0 to 1)
  - type: reset
    description: resets start/end from 0 to (current) array's size
  - type: continue <float>
    description: non-0 continue recording from where it last stopped
 - type: loop <float>
    description: non-0 enables loop recording, 0 disables it

draft: false
---

[tabwriter~] records up to 64 signal channels into arrays and can be triggered with sample accuracy.
