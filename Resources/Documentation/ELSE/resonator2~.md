---
title: resonator2~

description: resonant complex one pole filter

categories:
 - object

pdcategory: ELSE, Filters

arguments:
- type: float
  description: frequency
  default: 0
- type: float
  description: t60 decay time in ms
  default: 0

inlets:
  1st:
  - type: float/signal
    description: central frequency in Hertz
  2nd:
  - type: signal
    description: excitation signal
  3rd:
  - type: float/signal
    description: t60 decay time in ms (resonance)

outlets:
  1st:
  - type: signal
    description: real filtered signal
  2nd:
  - type: signal
    description: imaginary filtered signal

draft: false
---
[resonator2~] is a wrap around [cpole~]. It is very similar to [vcf~], which is also a wrap around [cpole~], but has no constant maximum gain at 0dB - so it works like [resonator~] with a constant skirt. Like [resonator~], it takes a decay time for resonance. Unlike [resonant~], you shouldn't excite it with noise as the filter produces very high gain output, so it's really suited just to be excited by impulses. By the way, this filter is the basis of [damp.osc~] and [resonbank2~].
