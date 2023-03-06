---
title: play~

description: array player

categories:
 - object

pdcategory: cyclone, Arrays and Tables

arguments:
- type: symbol
  description: table name (required)
  default:
- type: float
  description: number of output channels (1-64)
  default: 1

inlets:
  1st:
  - type: float
    description: play (non-0), stop (0) playback
  - type: signal
    description: playback position in ms

outlets:
  nth:
  - type: signal
    description: the playback of a channel
  2nd:
  - type: bang
    description: when it stops/finishes playing

methods: 
  - type: set <symbol>
    description: set array name
  - type: start <f, f, f>
    description: sets <beginning, end, duration> in ms and starts playing
  - type: stop
    description: stops playing and outputs 0 (cannot be resumed)
  - type: pause
    description: - pauses at a particular point (can be resumed)
  - type: resume
    description: resumes playing
  - type: loop <float>
    description: - <non-0> - on, <0> - off
  - type: loopinterp <float>
    description: <non-0> crossfade on, <0> crossfade off
  - type: interptime <float>
    description: - sets crossfading time in ms
    default: 50

flags:
  - name: @loop <float>
    description: loop mode on (1) or off (0)
  - name: @loopinterp <float>
    description: crossfade on (1) or off (0)
  - name: @interptime <float>
    description: crossfade time in ms

draft: true
---

[play~] plays any part of an array, it can play at different speeds (with interpolation) and backwards. It may also loop with optional crossfading. The interpolation is the same as [tabread4~]'s.
