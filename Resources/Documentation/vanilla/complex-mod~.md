---
title: complex-mod~
description: complex amplitude modulator
categories:
- object
see_also:
- hilbert~
pdcategory: vanilla, Effects, Signal Math
last_update: '0.52'
inlets:
  1st:
  - type: signal
    description: real part of signal input
  2nd:
  - type: signal
    description: imaginary part of signal input
  3rd:
  - type: float/signal
    description: frequency shift amount in Hz
outlets:
  1st:
  - type: signal
    description: frequency shifted side band
  2nd:
  - type: signal
    description: opposite side band (shifted in reverse)
draft: false
---
The complex modulator takes two signals in which it considers to be the real and imaginary part of a complex-valued signal. It then does a complex multiplication by a sinusoid to shift all frequencies up or down by any frequency shift in Hz.

This is also known as 'single side band modulation' and relates to ring modulation (which has two sidebands)

The left output is the frequency shifted by the amount of the frequency shift. The right outlet gives us the other side band, which is shifted by the same amount in reverse.

(for instance, if the shift is 100, left output shifts the frequency up by 100 and the right shifts it down by 100)
