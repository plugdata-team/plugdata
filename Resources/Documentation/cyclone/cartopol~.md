---
title: cartopol~
description: signal cartesian to polar conversion
categories:
 - object
pdcategory: cyclone, Converters
arguments:
inlets:
  1st:
  - type: signal
    description: real part of the complex signal in cartesian form
  2nd:
  - type: signal
    description: imaginary part of the complex signal in cartesian form
outlets:
  1st:
  - type: signal
    description: amplitude output of the polar form
  2nd:
  - type: signal
    description: phase in radians (-pi to pi) output of the polar form

draft: false
---

Use the [cartopol~] object to convert signal values representing cartesian coordinates to a signal composed of polar coordinates - useful for spectral processing.

