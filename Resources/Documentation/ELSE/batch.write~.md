---
title: batch.write~

description: table batch write

categories:
- object

pdcategory: ELSE, Buffers

arguments:
- description: array name to write to
  type: symbol
  default: none
- description: number of channels to record
  type: float
  default: 1, max 64

inlets:
  1st:
  - type: bang
    description: record the whole array
  - type: signal
    description: signal inputs to record for each channel
  nth:
  - type: signal
    description: signal inputs to record for each channel

outlets:
  1st:
  - type: bang
    description: outputs a bang when the object is done recording

methods:
  - type: set <symbol>
    description: sets array value

draft: false
---

[batch.write~] is a convenient abstraction based on [tabwriter~] and the 'fast-forward' message to pd that batch records to arrays (from an object like [sample~]).
