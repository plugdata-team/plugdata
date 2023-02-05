---
title: meter2~

description: stereo VU-meter

categories:
- object

pdcategory: ELSE, UI, Analysis

arguments:
- type: float
  description: window size for the [vu~] objects
  default:
- type: float
  description: hop size for the [vu~] objects
  default:

inlets:
  1st:
  - type: signal
    description: left incoming signal channels to be vu-metered
  2nd:
  - type: signal
    description: right incoming signal channels to be vu-metered

outlets:
  1st:
  - type: signal
    description: left incoming signals are passed through
  2nd:
  - type: signal
    description: right incoming signals are passed through
  3rd:
  - type: list
    description: vu values (RMS/peak amplitude in dBFS) of inputs

draft: false
---

[meter2~] is a convenient stereo VU-meter abstraction based on [vu~] and vanilla's [vu] GUI. See also [meter~], [meter4~] and [meter8~].
