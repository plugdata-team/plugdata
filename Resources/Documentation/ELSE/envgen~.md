---
title: envgen~

description: envelope generator

categories:
 - object

pdcategory: ELSE, Envelopes and LFOs

arguments:
- type: list
  description: <f> — initial output value, <f,f,..> sets the envelope
  default: 0 0

inlets:
  1st:
  - type: float/signal
    description: gate: on/attack (non-0) or off/release (0)
  - type: bang
    description: attacks the envelope with the last gate on value
  - type: list
    description: sets and runs envelope (pairs of duration & target. if odd, 1st float is starting point)
  2nd:
  - type: signals
    description: impulses to retrigger the envelope

outlets:
  1st:
  - type: signals
    description: envelope signal
  2nd:
  - type: float
    description: status: 1 <on>, 0 <off>

flags:
  - name: -exp <list>
    description: sets function with an extra exponential element for each segment
  - name: -init <float>
    description: default 0
  - name: -retrigger <float>
    description: default 0
  - name: -legato
    description: sets to legato mode on, default is off
  - name: -suspoint <float>
    description: default 0
  - name: curve <float/symbol>
    description: sets curve for all segments (default linear)

methods:
  - type: setgain <float>
    description: sets overall gain
  - type: curve <list>
    description: sets exponential values for each line segment
  - type: attack
    description: same as bang or gate on
  - type: release
    description: same as a gate off
  - type: pause
    description: pauses the output
  - type: resume
    description: resumes the envelope after being paused
  - type: suspoint <float>
    description: sets sustain point
  - type: retrigger <float>
    description: retrigger time in ms
  - type: legato <float>
    description: non-0 sets to legato mode
  - type: loop <float>
    description: non-0 sets to loop mode
  - type: samps <float>
    description: non-0 sets to samples instead of ms

draft: false
---

[envgen~] is an envelope (and an all purpose line) generator (here it creates a 1000 ms line to 1 and 500 ms line to 0).
