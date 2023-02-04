---
title: play.file~

description: play sound files

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
  - type: bang
    description: (re)start playing the sample
  - type: float
    description: non-0 (re)starts playing, zero stops

outlets:
  nth:
  - type: signal
    description: the sample signal of channel $nth
  2nd:
  - type: bang
    description: a bang when finishing playing the sample

flags:
  - name: -loop
    description: turns loop mode on

methods:
  - type: start
    description: (re)start playing the sample
  - type: stop
    description: stop playing the sample
  - type: open <symbol>
    description: opens a file with the symbol name (no symbol opens dialog box) and starts playing
  - type: set <symbol>
    description: sets a file to open next time it starts playing
  - type: loop <float>
    description: loop on <1> or off <0>

draft: false
---

[play.file~] is a convenient abstraction based on [readsf~] to read files from your computer. Note that the file sample needs to be at the same sample rate as Pd's. The object can open files with absolute paths and is also able to search for files relative to the parent patch.

