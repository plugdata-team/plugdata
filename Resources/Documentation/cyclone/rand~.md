---
title: rand~

description: Interplated bandlimited noise

categories:
 - object

pdcategory: General

arguments:
- type: float
  description: sets initial frequency
  default: 0

inlets:
  1st:
  - type: float/signal
    description: sets frequency

outlets:
  1st:
  - type: signal
    description: interpolated bandlimited noise

draft: true
---

[rand~] generates random values between -1 and 1 at a given frequency, interpolating linearly between the generated values. The resulting sound is a bandlimited noise.