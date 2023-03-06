---
title: overdrive~

description: analog overdrive simulation

categories:
 - object

pdcategory: cyclone, Effects

arguments:
- type: float
  description: initial drive factor
  default:

inlets:
  1st:
  - type: signal
    description: the signal to be distorted
  2nd:
  - type: signal
    description: drive factor

outlets:
  1st:
  - type: signal
    description: the distorted signal

draft: true
---

[overdrive~] simulates the "soft clipping" of and overdriven tube-based circuit by applying a non-linear transfer function to the incoming signal.
If the "drive factor" is 1, the signal is unchanged. Increasing the "drive" increases the amount of distortion. If the "drive" is less than 1, then it causes a different kind of distortion. If the "drive" is less than 0, VERY LOUD distortion can result, so be careful (here we use [clip~])!
