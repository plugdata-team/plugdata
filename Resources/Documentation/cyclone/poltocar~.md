---
title: poltocar~

description: signal polar to cartesian conversion

categories:
 - object

pdcategory: cyclone, Converters

arguments: (none)

inlets:
  1st:
  - type: signal
    description: amplitude of the signal in the polar form
  2nd:
  - type: signal
    description: phase (in radians) of the signal in the polar form

outlets:
  1st:
  - type: signal
    description: real part of the signal in the cartesian form
  2nd:
  - type: signal
    description: imaginary part of the signal in the cartesian form

draft: true
---

Use the [cartopol~] object to convert signal values representing cartesian coordinates to a signal composed of polar coordinates - useful for spectral processing.
