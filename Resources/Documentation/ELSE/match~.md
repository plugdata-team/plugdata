---
title: match~

description: compare signals to numbers

categories:
 - object

pdcategory: ELSE, Analysis, Mixing and Routing

arguments:
- type: list
  description: values to match to
  default: 0

inlets:
  1st:
  - type: signal
    description: the signal to be matched
  2nd:
  - type: list
    description: one or more floats reset argument values

outlets:
  nth:
  - type: signal
    description: impulse when signal matches an argument
  2nd:
  - type: signal
    description: impulse when signal doesn't match arguments

draft: false
---

[match~] is quite similar to pd vanilla's [select] object, but is signal based and sends impulses instead of bangs.
When a signal value corresponds to one of the arguments, an impulse is sent to its corresponding outlet. Otherwise, the rightmost outlet sends an impulse.
