---
title: sfload
description: load any sound file into an array

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
  - type: load <symbol>
    description: load a file into an array
  - type: set <symbol>
    description: set array name

outlets:

methods:

draft: false
---

[sfload] is similar to [soundfiler] but supports MP3, FLAC, WAV, AIF, AAC, OGG & OPUS audio files
