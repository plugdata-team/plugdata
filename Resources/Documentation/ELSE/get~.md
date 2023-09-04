---
title: get~

description: get channels from a multichannel connection

categories:
- object

pdcategory: ELSE, Mixing and Routing

arguments:
  - description: one or more channels to get
    type: list
    default: 0

inlets:
  1st:
  - type: signals
    description: multichannel signal to get stuff from
  - type: list
    description: channels to get

outlets:
  1st:
  - type: signal(s)
    description: one or more channels that you got

draft: false
---

[get~] gets one or more channels from a multichannel connection. The channels are indexed from 1 (0 or less gives "none", the max channel number value is clipped to the input's channel size). You can give it lists to build or remap the signal arbitrarily like you'd do wtih dollar signs in a message.

