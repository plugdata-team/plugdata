---
title: rev3~
description: hard-core, 2-in, 4-out reverberator
categories:
- object
see_also:
- rev1~
- rev2~
pdcategory: "'EXTRA' (patches and externs in pd/extra)"
last_update: '0.37.1'
inlets:
  1st:
  - type: signal
    description: left channel reverb input.
  2nd:
  - type: signal
    description: right channel reverb input.
  3rd:
  - type: float
    description: level in dB.
  4th:
  - type: float
    description: liveness (internal feedback percentage).
  5th:
  - type: float
    description: Crossover frequency in Hz.
  6th:
  - type: float
    description: High Freuqnecy damping in percentage.
outlets:
  1st:
  - type: signal
    description: first reverb output.
  2nd:
  - type: signal
    description: second reverb output.
  3rd:
  - type: signal
    description: third reverb output.
  4th:
  - type: signal
    description: fourth reverb output.
arguments:
- type: float
  description: level in dB (default 0).
- type: float
  description: liveness / internal feedback percentage (default 0).
- type: float
  description: Crossover frequency in Hz (default 3000).
- type: float
  description: High Frequnecy damping in percentage (default 0).
draft: false
---
(A more expensive, presumably better, one than rev2~.)

The creation arguments (output level, liveness, crossover frequency, HF damping) may also be supplied in four inlets as shown. The "liveness" (actually the internal feedback percentage) should be 100 for infinite reverb, 90 for longish, and 80 for short. The crossover frequency and HF damping work together: at frequencies above crossover, the feedback is diminished by the "damping" as a percentage. So zero HF damping means equal reverb time at all frequencies, and 100% damping means almost nothing above the crossover frequency gets through.