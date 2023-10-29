---
title: slice~

description: split a multichannel signal

categories:
 - object

pdcategory: ELSE, Mixing and Routing

arguments:
- type: float
  description: slice point
  default: 0

inlets:
  1st:
  - description: multichannel signal to slice
    type: signals
  - description: slice point
    type: float

outlets:
  1st:
  - description: the left output of the sliced signal
    type: signal
  2nd:
  - description: the righft output of the sliced signal
    type: signal

draft: false
---

[slice~] splits a multichannel signal. The 'n' split point is set via argument or float input. If you slice at 'n', the left outlet spits the first 'n' channels and the right outlet sends the remaining channels. If you slice at '-n', the right outlet spits the last 'n' channels and the left outlet sends the first channels. If the number of channels is less than the split point, the whole signal is output to the left if 'n' is positive (and the right outlet outputs zeros), and to the right if negative (and the left outlet outputs zeros).
