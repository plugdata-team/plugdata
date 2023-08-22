---
title: sum~

description: sum multichannel signals into a single channel

categories:
- object

pdcategory: ELSE, Mixing and Routing

arguments:
  - description: non zero sums, zero passes unchanged
    type: float
    default: non zero

inlets:
  1st:
  - type: signal(s)
    description: incoming multichannel connection to sum

outlets:
  1st:
  - type: signal(s)
    description: summed (mono) or unsummed multichannel signals

methods:
- type: sum <float>
  description: non zero sums to a single channel, zero passes unchanged

draft: false
---

[sum~] takes a multisignal connection and sums them into a single mono channel.

