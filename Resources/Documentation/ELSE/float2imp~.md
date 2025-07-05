---
title: float2imp~
description: convert floats to pulse

categories:
 - object

pdcategory: ELSE, Data Math, Converters

arguments:
- type: float
  description: impulse value
  default: 1

inlets:
  1st:
  - type: float
    description: set impulse value and generate impulse
  - type: bang
    description: generate impulse
  2nd:
  - type: float
    description: set impulse value

outlets:
  1st:
  - type: signal
    description: impulses from bangs

draft: false
---

[float2imp~] converts floats to impulses with sample accuracy. It is an abstraction based on [vline~], so it can convert to an impulse within an audio block. A bang can be used to output the impulse with the stored value. The default value is '1' and can be set via the argument. The right inlet also sets the impulse value. A float input sets the value and gets converted to an impulse.
