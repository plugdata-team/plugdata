---
title: rescale

description: rescale MIDI values

categories:
 - object

pdcategory: ELSE, MIDI, Data Math

arguments:
- type: float
  description: minimum output value
  default: 0
- type: float
  description: maximum output value
  default: 1
- type: float
  description: exponential value
  default: 0, linear

inlets:
  1st:
  - type: list
    description: original value(s)
    
  2nd:
  - type: float
    description: minimum output value
      
   3rd:
    - type: float
      description: maximum output value

outlets:
  1st:
  - type: list
    description: the rescaled value(s)

flags:
  - name: -noclip
    description: sets clipping off
  - name: -in <float, float> 
    description: sets min/max input values
  - name: -exp <float> 
    description: sets exponential factor
  - name: -noclip
    description: sets to log mode

methods:
  - type: exp <float>
    description: sets the exponential factor, -1, 0 or 1 sets to linear
  - type: clip <float>
    description: non zero sets clipping on, 0 sets it off
   - type: log <float>
    description: non zero sets to log mode

draft: false
---

By default, [rescale] rescales MIDI input values from 0 to 127 (including floats) into another range of values (0-1 by default). You can also set to log or exponential factor (0 by default - linear).
