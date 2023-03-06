---
title: player~

description: multi-channel player

categories:
- object

pdcategory: ELSE, Buffers

arguments:
- description: (optional) number of channels (max 64)
  type: float
  default: 1 if no file is given, or sound file's if given
- description: the name of the file to open
  type: symbol
  default: none
- description: autostart <1: on, 0: off>
  type: float
  default: 0
- description: loop <1: on, 0: off>
  type: float
  default: 0

inlets:
  1st:
  - type: float
    description: non-0 plays, <0> stops
  - type: bang
    description: play (same as non-0)

outlets:
  nth:
  - type: signal
    description: the buffer signal of channel $nth
  2nd:
  - type: bang
    description: a bang when finishing playing the buffer

flags:
  - name: -speed <float>
    description: (default 100)
  - name: -range <f, f>
    description: (default 0 1)
  - name: -fade <float>
    description: (default 0)
  - name: -xfade
    description: xfade mode on
  - name: -loop
    description: loop mode on

methods:
  - type: open <symbol>
    description: opens a file (no symbol opens dialog box) and plays
  - type: set <symbol>
    description: sets a file to open (needs a reload message)
  - type: show
    description: shows the buffer subpatch window
  - type: hide
    description: hides the buffer subpatch window
  - type: start <float>
    description: sets start point in ms
  - type: end <float>
    description: sets end point in ms
  - type: range <f, f>
    description: sets start and end point proportionally (from 0 to 1)
  - type: speed <float>
    description: sets playing speed in %
  - type: pos <float>
    description: sets position (from 0 to 1) and starts playing it
  - type: play <f, f, f>
    description: if no float is given, plays whole file. 1st float sets start, 2nd sets end (in ms) and 3rd sets speed rate
  - type: <stop>
    description: stops playing and outputs 0 (cannot be resumed)
  - type: <pause>
    description: pauses at a particular point (can be resumed)
  - type: <resume>
    description: resumes playing after being paused
  - type: fade <float>
    description: sets fading time in ms (default 0)
  - type: xfade <float>
    description: non-0 sets to crossfading mode (default 0)
  - type: loop <float>
    description: non-0 enables looping, <0> disables it (default 0)

draft: false
---

[player~] is a convenient abstraction based on [soundfiler] and [tabplayer~]. It's much more versatile than [play.file~]. For instance, you can play in different speeds and backwards.

