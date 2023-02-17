---
title: tabreader

description: table reader

categories:
- object

pdcategory: ELSE, Arrays and Tables

arguments:
  - type: symbol
    description: array name (optional)
    default: none


inlets:
  1st:
  - type: float
    description: sets index/phase

outlets:
  1st:
  - type: signal
    description: table values

flags:
  - name: -lin
    description: sets interpolation type
  - name: -cos
    description: sets interpolation type
  - name: -cubic
    description: sets interpolation type
  - name: -lagrange
    description: sets interpolation type
  - name: -hermite <f, f>
    description: sets interpolation type
  - name: -none
    description: sets interpolation type
  - name: -ch <float>
    description: set initial loaded channel (default 1)
  - name: -index
    description: sets to indexed mode
  - name: -loop
    description: sets to loop mode

methods:
    - type: set <symbol>
      description: sets an entire array to be used as a waveform
    - type: index <float>
      description: non-0 sets index to read from
    - type: loop <float>
      description: non-0 sets loop mode
    - type: none
      description: sets to no interpolation
    - type: lin
      description: sets to linear interpolation
    - type: cos
      description: sets to cosine interpolation
    - type: cubic
      description: sets to cubic interpolation
    - type: lagrange
      description: sets to Lagrange interpolation
    - type: hermite <f, f>
      description: sets to Hermite interpolation


draft: false
---

[tabreader] accepts indexes from 0 to 1 by default and reads an array with different interpolation methods with multi channel support. There's no need to have guard points in the array as these are taken care of internally.
