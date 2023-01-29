---
title: envgen~

description: envelope generator

categories:
 - object

pdcategory: Envelopes and LFOs

arguments:
- type: list
  description: a float sets an initial output value, a list of floats sets the envelope
  default: 0 0

inlets:
  1st:
  - type: float/signal
    description: gate: on/attack (non-0) or off/release (0)
  - type: bang
    description: attacks the envelope with the last gate on value
  - type: list
    description: sets and runs envelope. Lists are pairs of duration & target. If odd, 1st float is the starting point
  2nd:
  - type: list
    description: sets the envelope and doesn't run it

outlets:
  1st:
  - type: signal
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
  - name: -maxsustain <float>
    description: default 0 - no maxsustain
  - name: -suspoint <float>
    description: default 0

methods:
  - type: set <list>
    description: sets the envelope and doesn't run it
  - type: setgain <float>
    description: sets overall gain
  - type: exp <list>
    description: sets envelope with an extra exponential element for each segment
  - type: expl <list>
    description: sets exponential values for each line segment
  - type: expi <f, f>
    description: sets an exponential for a line segment specified by the first float indexed from 0
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
  - type: maxsustain <float>
    description: sets maximum sustain length in ms
  - type: retrigger <float>
    description: retrigger time in ms
  - type: legato <float>
    description: non-0 sets to legato mode

---

[envgen~] is an envelope (and an all purpose line) generator (here it creates a 1000 ms line to 1 and 500 ms line to 0).

