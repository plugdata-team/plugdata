---
title: nchs~

description: get number of channels from multi-channel connections

categories:
- object

pdcategory: ELSE, Mixing and Routing

arguments:

inlets:
  1st:
  - type: signals
    description: signal to query number of channels
  - type: bang
    description: query number of channels

outlets:
  1st:
  - type: float
    description: the number of channels

draft: false
---

[nchs~] gets the number of channels from a multi-channel connection, as provided by [snake~ in], [clone] or other objects. The object outputs values whenever the DSP chain gets updated and you can also query for the number of channels with a bang message. It also works in subpatches or abstractions because [inlet~] is also multi-channel aware.
