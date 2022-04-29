---
title: fiddle~
description: pitch estimator and sinusoidal peak finder
categories:
- object
see_also:
- sigmund~
pdcategory: "Obsolete"
last_update: '?'
inlets:
  1st:
  - type: signal
    description: signal input to be analyzed.
  - type: amp-range <float, float>
    description: set low and high amplitude threshold
  - type: vibrato <float, float>
    description:  set period in msec and pitch deviation for "cooked" pitch outlet.
  - type: reattack <float, float>
    description:  set period in msec and amplitude in dB to report re-attack.
  - type: npartial <float>
    description:  set partial number to be weighted half as strongly as the fundamental.
  - type: auto <float>
    description: nonzero sets to auto mode 
  default:
 and zero sets it off.
  - type: bang
    description: poll current values (useful when auto mode is off