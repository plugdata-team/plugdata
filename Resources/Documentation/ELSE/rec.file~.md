---
title: rec.file~

description: record sound file

categories:
- object

pdcategory: ELSE, Buffers

arguments:
- type: symbol
  description: (optional) name of the file to save to
  default: file.wav
- type: float
  description: number of channels to record
  default: 1

inlets:
  1st:
  - type: start 
    description: start recording
  - type: stop
    description: stop recording
  - type: float
    description: non-0 starts playing, 0 stops

  nth:
  - type: signal
    description: signal inputs to record for each channel

outlets:
  1st:
  - type: bang
    description: outputs a bang when stops recording

methods:
- type: set <symbol>
  description: sets file name (no symbol opens dialog box)
- type: rate <float>
  description: sets sample rate
- type: fade <float>
  description: sets fade time (minimum 1)
- type: maxtime <float>
  description: sets maximum recording in ms
 
draft: false
---

[rec.file~] is a convenient abstraction based on [writesf~], so it records sample files that you can load with other objects, such as [sample~], [play.file~], [player~] and others.
