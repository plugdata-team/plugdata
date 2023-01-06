---
title: pique
description: find peaks in an FFT spectrum
categories:
- object
see_also:
- sigmund~
pdcategory: "Obsolete"
last_update: '0.31'
inlets:
  1st:
  - type: list
    description: number of FFT points, table name for real part, table name forimaginary part and maximum number of peaks to report.
outlets:
  1st:
  - type: list
    description: partial index, frequency in hz, amplitude, cosine and sine components.
draft: false
---
NOTE: pique is obsolete! consider using [sigmund~]

pique takes unwindowed FFT analyses as input (they should be stored in arrays) and outputs a list of peaks, giving their peak number, frequency, amplitude, and phase (as a cosine/sine pair.)