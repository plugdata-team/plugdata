---
title: phasewrap~

description: wrap between pi & -pi

categories:
 - object

pdcategory: cyclone, Signal Math

arguments: (none)

inlets:
  1st:
  - type: signal
    description: signal to be wrapped

outlets:
  1st:
  - type: signal
    description: wrapped signal

draft: true
---

Use [phasewrap~] ro wrap the input between π (pi) and -π (-pi) - useful for wrapping phase values. When the input exceeds π (around 3.14159), the output signal value is "wrapped" to a range whose lower bound is -π (around -3.14159).
