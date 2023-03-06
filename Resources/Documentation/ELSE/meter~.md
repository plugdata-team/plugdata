---
title: meter~

description: mono VU-meter

categories:
- object

pdcategory: ELSE, UI, Analysis

arguments:
- type: float
  description: window size for the [vu~] object
  default:
- type: float
  description: hop size for the [vu~] object
  default:

inlets:
  1st:
  - type: signal
    description: incoming mono signal to be vu-metered

outlets:
  1st:
  - type: signal
    description: incoming signal is passed through
  2nd:
  - type: list
    description: vu values (RMS/peak amplitude in dBFS)

draft: false
---

[meter~] is a convenient mono VU-meter abstraction based on [vu~] and vanilla's [vu] GUI object. See also [meter2~], [meter4~] and [meter8~].
