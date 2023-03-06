---
title: onebang

description: gate a bang with a bang

categories:
 - object

pdcategory: cyclone, Data Management

arguments:
- type: float
  description: non-0 lets the first bang on left inlet through
  default:

inlets:
  1st:
  - type: anything
    description: any message is treated as a bang
  - type: bang
    description: a bang to be gated
  2nd:
  - type: anything
    description: any message is treated as a bang
  - type: bang
    description: allows a bang in the left inlet through left outlet once

outlets:
  1st:
  - type: bang
    description: if a bang was sent before to the right inlet
  2nd:
  - type: bang
    description: outputs a bang otherwise

draft: true
---

[onebang] allows a bang in the left inlet to pass through the left outlet once ONLY if a bang has been previously received in the right inlet. The bang goes to the right outlet otherwise.
