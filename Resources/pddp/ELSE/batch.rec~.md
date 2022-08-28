---
title: batch.rec~

description: Sound file batch record

categories:
- object

pdcategory: General Audio Manipulation

arguments:
  1st:
  - description:  (optional) file name to save to
    type: symbol
    default: file.wav
  2nd:
  - description: number of channels to record
    type: symbol
    default: 1, max 64

inlets:
  1st:
  - type: rec <float>
    description: starts recording for given amount of time in ms. If no float is given, the last set value is used
  - type: set <symbol>
    description: sets file name (no symbol opens dialog box)
  Nth:
  - type: signal
    description: signal inputs to record for each channel

outlets:
  1st:
  - type: bang
    description: outputs a bang when the object is done recording

click:
  - description: clicking on the object opens a dialog box to set a file to save to

draft: false
---

[batch.rec~] is a convenient abstraction based on [writesf~] and the 'fast-forward' message to pd that batch records to a sound file that you can load with objects like [sample~], [play.file~], [player~] and others.
