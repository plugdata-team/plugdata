---
title: poke~

description: write to an array

categories:
 - object

pdcategory: cyclone, Arrays and Tables

arguments:
- type: symbol
  description: array name to write to
  default:
- type: float
  description: channel (1-64)
  default: 1

inlets:
  1st:
  - type: float/signal
    description: values to write into an array
  - type: list <f, f, f>
    description: <value>, <index> and <channel>
  2nd:
  - type: float/signal
    description: index to record to
  3rd:
  - type: float
    description: sets the channel for the value being recorded

outlets:

methods:
  - type: set <symbol>
    description: set array name

draft: true
---

[poke~] writes signals to an array at indexes specified by a signal.
