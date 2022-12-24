---
title: rescale~

description: Rescale audio

categories:
 - object

pdcategory: Math

arguments:
- type: list
  description: 3 arguments: minimum output, maximum output, exponential factor. 5: minimum input, maximum input, minimum output, maximum output, exponential factor
  default: 0, 127, 0, 1, 1

inlets:
  1st:
  - type: signal
    description: value to perform the scaling function on
  nth:
  - type: signal
    description: range parameters

outlets:
  1st:
  - type: signal
    description: the rescaled signal

flags:
  - name: -clip
    description: sets clipping on
    
methods:
  - type: exp <float>
    description: sets the exponential factor
  - type: clip <float>
    description: 1 - clipping in, 0 - off


draft: false
---

By default, [rescale~] rescales input values from -1 to 1 into another range of values (0-1 by default). You can also set an exponential factor (1 by default - linear). All these parameters can be changed by arguments.