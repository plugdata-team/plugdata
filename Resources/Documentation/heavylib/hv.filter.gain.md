---
title: hv.filter.gain

description: generic filter module with swappable filter types and frequency, Q and gain settings

categories:
- object

pdcategory: heavylib, Filters

arguments:
- type: symbol
  description: filter type <highshelf, lowshelf, or peak>
- type: float
  description: frequency
- type: float
  description: Q

inlets:
  1st:
  - type: signal
    description: input signal
  2nd:
  - type: float
    description: frequency
  3rd:
  - type: float
    description: Q
  4th:
  - type: float
    description: gain

outlets:
  1st:
  - type: signal
    description: filtered signal

draft: false
---

