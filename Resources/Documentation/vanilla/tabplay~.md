---
title: tabplay~
description: play a table as a sample (non-transposing)
categories:
- object
see_also:
- tabwrite~
- tabread4~
- tabread
- tabwrite
- soundfiler
- array
pdcategory: vanilla, Signal Generators, Arrays and Tables, Buffers
last_update: '0.43'
inlets:
  1st:
  - type: float
    description: sets starting sample and plays the sample
  - type: bang
    description: plays the whole sample (same as '0')
  - type: list
    description: 1st element sets starting sample and 2nd element sets duration in samples
outlets:
  1st:
  - type: signal
    description: sample output
  2nd:
  - type: bang
    description: bang when finished playing the table
arguments:
  - type: symbol
    description: sets table name with the sample
methods:
  - type: stop
    description: stop playing (outputs zeros when stopped)
  - type: set <symbol>
    description: set the table with the sample
draft: false
---
The tabplay~ object plays a sample, or part of one, with no transposition or interpolation. It is cheaper than tabread4~ and there are none of tabread4~'s interpolation artifacts.
