---
title: bob~
description: Runge-Kutte numerical simulation of the Moog analog resonant filter
categories:
- object
see_also:
- lop~
- hip~
- bp~
- vcf~
- biquad~
- slop~
- cpole~
- fexpr~
pdcategory: "'EXTRA' (patches and externs in pd/extra)"
last_update: '0.52'
inlets:
  1st:
  - type: float/signal
    description: input to be filtered.
  - type: saturation <float>
    description: sent saturation point of "transistors".
  - type: oversample <float>
    description: sent oversampling.
  - type: clear
    description: clear internal state.
  - type: print
    description: post internal state and parameters on Pd's window.
  2nd:
  - type: float/signal
    description: resonant or cutoff frequency in hertz.
  3rd:
  - type: float/signal
    description: resonannce.
outlets:
  1st:
  - type: signal
    description: filtered signal.
draft: false
---
The design of bob~ is based on papers by Tim Stilson, Timothy E. Stinchcombe, and Antti Huovilainen. See README.txt for pointers. The three audio inputs are the signal to filter, the cutoff/resonant frequency in cycles per second, and "resonance" (the sharpness of the filter). Nominally, a resonance of 4 should be the limit of stability -- above that, the filter oscillates.

By default bob~ does one step of 4th-order Runge-Kutte integration per audio sample. This works OK for resonant/cutoff frequencies up to about 1/2 Nyquist. To improve accuracy and/or to extend the range of the filter to higher cutoff frequencies you can oversample by any factor - but note that computation time rises accordingly. At high cutoff frequencies/resonance values the RK approximation can go unstable. You can combat this by raising the oversampling factor.

The saturation parameter determines at what signal level the "transistors" in the model saturate. The maximum output amplitude is about 2/3 of that value. "Clear" momentarily shorts out the capacitors in case the filter has gone unstable and stopped working.

Compatibility note: there was a bug in this module, fixed for Pd version 0.52. You can get the (incorrect) pre-0.52 behavior by setting pd's compatibility level to 0.51.