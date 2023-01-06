---
title: writesf~
description: write audio signals to a soundfile
categories:
- object
see_also:
- soundfiler
- readsf~
pdcategory: General Audio Manipulation
last_update: '0.51'
inlets:
  1st:
  - type: signal
    description: signal to write to a channel.
  - type: open <list>
    description: takes a filename and optional flags -wave, -aiff, -caf, -next, - big, -little, -bytes <float>, -rate <float>
  - type: start
    description: start streaming audio.
  - type: stop
    description: stop streaming audio
  - type: print
    description: prints information on Pd's terminal window.
  nth:
  - type: signal
    description: signal to write to a channel.
arguments:
- type: float
  description: sets number of channels
  default: 1
draft: false
---
writesf~ creates a subthread whose task is to write audio streams to disk. You need not provide any disk access time between "open" and "start", but between "stop" and the next "open" you must give the object time to flush all the output to disk.

The "open" message may take flag-style arguments as follows:

- -wave, -aiff, -caf, -next (file extension
