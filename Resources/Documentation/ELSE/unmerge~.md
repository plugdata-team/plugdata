---
title: unmerge~

description: split a multichannel signal

categories:
 - object

pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: number of outlets, there's an extra outlet for the extra channels
  default: 2
- type: float
  description: channel number per outlet
  default: 1

inlets:
  1st:
  - description: multichannel signal to slice
    type: signals
  - description: number of channels in a group
    type: float

outlets:
  1st:
  - description: channel group of a multichannel signal
    type: signal
  nth:
  - description: extra channel group of a multichannel signal
    type: signal

draft: false
---

[unmerge~] separates the channels of a multichannel signal in groups of any size (default 1). Each group is sent out a separate outlet, extra elements are sent to an extra rightmost outlet. If the number of channels is less than the number of outlets, zeroes are output.