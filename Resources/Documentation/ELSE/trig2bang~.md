---
title: trig2bang~

description: Trigger to bang conversion

categories:
- object

pdcategory:

arguments:
- description:
  type:
  default:

inlets:
  1st:
  - type: signal
    description: signal to detect triggers

outlets:
  1st:
  - type: bang
    description: when detecting zero to non-zero transitions

draft: false
---

[trig2bang~] detects zero to non-zero transitions. This can be used to detect and convert trigger signals (impulses and gate) to a bang.