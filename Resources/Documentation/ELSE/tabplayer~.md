---
title: tabplayer~

description: array/table player

categories:
- object

pdcategory: ELSE, Arrays and Tables, Buffers

arguments:
  - type: symbol
    description: table name (optional)
    default:
  - type: float
    description: number of output channels (maximum 64)
    default: 1

flags:
  - name: -fade <float>
    description:
  - name: -speed <float>
    description:
  - name: -tr
    description: sets to trigger mode
  - name: -loop
    description: sets to loop mode
  - name: -tr
    description: sets to trigger mode
  - name: -loop
    description: sets to loop mode
  - name: -xfade
    description: sets to crossfade mode
  - name: -sr <float>
    description:
  - name: -range <f, f>
    description:

inlets:
  1st:
  - type: float
    description: non-0 plays, 0 stops
  - type: bang
    description: play (same as non-0)
  - type: signal
    description: gate on/impulse — starts, gate off — stops (not in trigger mode)

outlets:
  1st:
  - type: signal
    description: the playback of a channel
  2nd:
  - type: bang
    description: when it stops/finishes playing or when looping

methods:
  - type: tr <float>
    description: non-0 sets to trigger mode, zero sets to gate mode
  - type: set <symbol>
    description: sets array name
  - type: start <float>
    description: sets start point in ms
  - type: end <float>
    description: sets end point in ms
  - type: range <f, f>
    description: sets start and end point range proportionally (from 0 to 1)
  - type: pos <f>
    description: sets position proportionally within range (from 0 to 1)
  - type: reset
    description: resets range from 0 to array size
  - type: speed <float>
    description: sets playing speed in % 
    default: 100
  - type: play <f, f, f>
    description: start playing, optional <start in ms, end in ms, speed rate>
  - type: <stop>
    description: stops playing and outputs 0 (cannot be resumed)
  - type: <pause>
    description: pauses at a particular point (can be resumed)
  - type: <resume>
    description: resumes playing after being paused
  - type: loop <float>
    description: non-0 enables looping, <0> disables it 
    default: 0
  - type: fade <float>
    description: sets fade time in ms 
    default: 0
  - type: xfade <float>
    description: sets to crossfade mode when looping 
    default: no crossfade
  - type: sr <float>
    description: sets sample rate of sample (default, Pd's sample rate)

draft: false
---

[tabplayer~] plays arrays, it's more powerful than [tabplay~] as it has multichannel support and can play backwards and in different speeds. It can also loop.
