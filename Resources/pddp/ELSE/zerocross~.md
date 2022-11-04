---
title: zerocross~

description: Send impulses at zero crossings

categories:
 - object
 
pdcategory: Analysis

arguments: (none)

inlets: 
  1st:
  - type: signal
    description: signal to detect zero crossings

outlets:
  1st:
  - type: signal
    description: impulse when crossing from non-positive to positive
  2nd:
  - type: signal
    description: impulse when crossing from positive to non-positive
  3rd:
  - type: signal
    description: impulse when crossing either way

draft: false
---

[zerocross~] sens impulses when crossing from non-positive to positive (left outlet), from positive to non-positive (middle outlet) and both (right outlet).
