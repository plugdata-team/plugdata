---
title: detect~
description: period/frequency detection

categories:
 - object

pdcategory: ELSE, Analysis

arguments:
- type: symbol
  description: detection type: <samps>, <bpm>, <ms>, and <hz>
  default: <samps>

inlets:
  1st:
    - type: signal
      description: any signal to trigger the object

outlets:
  1st:
    - type: signal
      description: detected period/frequency

draft: false
---

[detect~] detects the frequency/period between trigger events. A trigger happens on a transition from non positive to positive.

