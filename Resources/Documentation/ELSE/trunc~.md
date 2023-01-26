---
title: trunc~

description: truncate to integer

categories:
- object

pdcategory: Signal Math

arguments:
- description:
  type:
  default:

inlets:
  1st:
  - type: signal
    description: signal input

outlets:
  1st:
  - type: signal
    description: truncated integer signal

draft: false
---

[trunc~] truncates a signal towards zero, that means only the integer value is considered.