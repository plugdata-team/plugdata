---
title: tap

description: Tapped tempo

categories:
- object

pdcategory:

flags:
- name: -ms
  description: sets output to ms (default bpm)

inlets:
  1st:
  - type: bang
    description: tap for tempo

outlets:
  1st:
  - type: float
    description: tapped tempo

draft: false
---

The [tap] object detects a tapped tempo by analyzing the time distance between two bangs or the time a gate is opened.