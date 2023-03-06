---
title: buffer~
description: audio buffer
categories:
 - object
pdcategory: cyclone, Buffers
arguments:
- type: symbol
  description: buffer name
- type: symbol
  description: audio file to load (optional)
- type: float
  description: buffer size in ms
  default: 0 or -1 if audio file is given
- type: float
  description: channels (1 to 64)
  default: default 1 or audio file's (if given)
inlets:
  1st:
  - type: open
    description: opens buffer supbatch window
outlets:

flags:
  - name: @size <float>
    description: sets buffer size in ms
  - name: @samps <float>
    description: sets buffer size in samples

methods:
  - type: clear
    description: fills all arrays with zeros
  - type: crop <f, f>
    description: trims the sample to a range in ms and resizes the buffer
  - type: read <symbol>
    description: sets file to load from (no symbol opens dialog box)
  - type: readagain
    description: reads and loads the previously opened file again
  - type: write <symbol>
    description: sets file to write to (no symbol opens dialog box)
  - type: format <any>
    description: sets bit depth - see [pd read/write] for details
  - type: writewave <symbol>
    description: sets file to write in wave (no symbol opens dialog box)
  - type: writeaiff <symbol>
    description: sets file to write in aiff (no symbol opens dialog box)
  - type: filetype <symbol>
    description: symbol sets file type to write ("wave" or "aiff")
  - type: open
    description: opens buffer supbatch window
  - type: wclose
    description: closes buffer supbatch window
  - type: sr <float>
    description: sets sample rate for writing a file (default: Patch's)
  - type: fill <any>
    description: see [pd function/generators] above
  - type: apply <any>
    description: see [pd function/generators] above
  - type: set <symbol>
    description: changes the buffer name
  - type: name <symbol>
    description: same as 'name'
  - type: normalize <f>
    description: normalizes to the given float value
  - type: setsize <float>
    description: sets buffer size in ms
  - type: sizeinsamps <f>
    description: sets buffer size in samples

draft: false
---

[buffer~] stores audio in a memory buffer. It read/writes multichannel audio files and can be used in conjunction with [play~] and other related objects. This is an abstraction without the full functionalities from the Max/MSP original.

