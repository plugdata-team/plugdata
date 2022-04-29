---
title: readsf~
description: read a soundfile
categories:
- object
see_also:
- soundfiler
- writesf~
pdcategory: General Audio Manipulation
last_update: '0.51'
inlets:
  1st:
  - type: open <list>
    description: sets a filename, an onset in samples, header size to skip, number of channels, bytes per sample, and endianness.
  - type: float
    description: nonzero starts playback, zero stops.
  - type: print
    description: prints information on Pd's terminal window.
outlets:
  'n':
  - type: signal
    description: channel output of a given file.
  rightmost:
  - type: bang
    description: when finishing playing file.
arguments:
- type: float
  description: sets number of output channels (default 1, max 64).
- type: float
  description: per channel buffer size in bytes.
draft: false
---
The readsf~ object reads a soundfile into its signal outputs. You must open the soundfile in advance (a couple of seconds before you'll need it) using the "open" message. The object immediately starts reading from the file, but output will only appear after you send a "1" to start playback. A "0" stops it.

The wave, aiff, caf, and next formats are parsed automatically, although only uncompressed 2- or 3-byte integer ("pcm") and 4-byte floating point samples are accepted.

Open takes a filename, an onset in sample frames, and, as an override, you may also supply a header size to skip, a number of channels, bytes per sample, and endianness.