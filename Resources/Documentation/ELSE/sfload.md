---
title: sfload
description: load any sound file into an array

categories:
- object

pdcategory: ELSE, Buffers

arguments:
  - description: the name of the array to load into
    type: symbol
    default: none

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
