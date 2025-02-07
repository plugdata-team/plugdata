---
title: vcf2~

description: resonant complex one pole filter

categories:
 - object

pdcategory: ELSE, Filters

arguments:
- type: float
  description: frequency
  default: 0
- type: float
  description: decay time in ms
  default: 0

inlets:
  1st:
  - type: signal
    description: filter input
  2nd:
  - type: float/signal
    description: frequency
  3rd:
  - type: float/signal
    description: t60 decay time in ms (resonance)

outlets:
  1st:
  - type: signal
    description: real part of the filter
  2nd:
  - type: signal
    description: imaginary part of the filter

draft: false
---
[vcf2~] is a wrap around [cpole~] just like [vcf~], but has no constant maximum gain at 0dB, so it has a constant skirt and resonates (kinda like [resonant~]). Unlike [vcf~], it takes a decay time for resonance, and it can be a signal. Unlike [resonant~], you shouldn't filter constant signals as the filter produces very high gain output, so it's really suited just to be excited by impuses. By the way, this filter is the basis of [damp.osc~].
