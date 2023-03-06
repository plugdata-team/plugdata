---
title: seq

description: MIDI sequencer

categories:
 - object

pdcategory: cyclone, MIDI, Triggers and Clocks

arguments:
- type: symbol
  description: a MIDI/text file to be loaded
  default:

inlets:
  1st:
  - type: bang
    description: starts or restarts playing a sequence at normal speed
  - type: float
    description: raw MIDI data stream to be recorded 

outlets:
  1st:
  - type: float
    description: raw MIDI data stream from played sequence
  2nd:
  - type: bang
    description: sent at the end of a sequence (useful for looping)

methods:
  - type: read <symbol>
    description: reads from MIDI/text files (no symbol opens a dialog box)
    default:
  - type: start <float>
    description: start/restart sequence at a given tempo
    default: 1024
  - type: stop
    description: stops recording/playing and goes back to start
    default:
  - type: record 
    description: starts recording
    default:
  - type: append
    description: records at the end of the stored sequence
    default:
  - type: clear
    description: clears the stored sequence
    default:
  - type: tick
    description: sends a bang at each tick
    default: external clock, needed after 'start -1' message
  - type: write <symbol>
    description: saves to MIDI/text files 
    default:
  - type: delay <float>
    description: onset time in ms of the first event
    default: 
  - type: addeventdelay <float>
    description: adds (in ms) to the onset delay
    default:
  - type: hook <float>
    description: multiplies all the event times by the given number
    default:
  - type: print
    description: prints sequence's first 16 events in the Pd window
    default:
  - type: pause
    description: pauses playing
    default:
  - type: continue
    description: continues playing
    default:

draft: true
---

[seq] plays/records raw MIDI streams and can save/read MIDI sequence files. All tracks of a multi-track midi file are merged into one.
