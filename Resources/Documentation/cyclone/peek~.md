---
title: peek~

description: read/write to array

categories:
 - object

pdcategory: cyclone, Arrays and Tables

arguments:
- type: symbol
  description: array name 
  default:
- type: float
  description: channel (1-64)
  default:
- type: float
  description: <0/1> - enable/disable clipping (to -1/1 range)
  default:

inlets:
  1st:
  - type: float
    description: index to read from or write to table
  - type: list <f, f, f>
    description: writes <value>, <index> and <channel> to table
  2nd:
  - type: float
    description: value to write to table (needs index at left inlet)
  3rd:
  - type: float
    description: channel number

outlets:
  1st:
  - type: float
    description: value corresponding to given array index


methods:
  - type: set <symbol>
    description: set array name
  - type: clip <float>
    description: - enable <1> or disable <0> clipping into -1 to 1 range

draft: true
---

[peek~] reads (without interpolation) and write values to an array via messages. Thus, not a proper signal object as it can't handle signals and works even when the DSP is off!
