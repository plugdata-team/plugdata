---
title: readsf~
description: read a soundfile
categories:
- object
see_also:
- soundfiler
- writesf~
pdcategory: vanilla, File Management, Buffers
last_update: '0.51'
inlets:
  1st:
  - type: float
    description: non-0 starts playback, zero stops
  - type: print
    description: prints information on Pd's terminal window
outlets:
  nth:
  - type: signal
    description: channel output of a given file
  2nd:
  - type: bang
    description: when finishing playing file
arguments:
- type: float
  description: sets number of output channels 
  default: 1
- type: float
  description: per channel buffer size in bytes
methods:
  - type: open <list>
    description: sets a filename, an onset in samples, header size to skip, number of channels, bytes per sample, and endianness
draft: false
---
The readsf~ object reads a soundfile into its signal outputs. You must open the soundfile in advance (a couple of seconds before you'll need it
