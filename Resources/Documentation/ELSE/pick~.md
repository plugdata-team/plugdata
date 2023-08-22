---
title: pick~

description: pick a channel from a multichannel connection

categories:
- object

pdcategory: ELSE, Mixing and Routing

arguments:
  - description: channel to pick
    type: float
    default: 0
 
inlets:
  1st:
  - type: signals
    description: multichannel signals to pick from
  - type: float
    description: channel to pick

outlets:
  1st:
  - type: signal
    description: picked channel

draft: false
---

[pick~] picks a channel from a multichannel connection. The channel is specified via argument or float input and is indexed from 1 (0 gives "none"). Negative values count from last to the first element. If you ask for an element number out of the range, you also get "none".

