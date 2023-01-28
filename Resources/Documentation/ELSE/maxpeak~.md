---
title: maxpeak~

description: maximum peak amplitude

categories:
- object

pdcategory: Analysis

arguments: (none)

inlets:
  1st:
  - type: singal
    description: incoming signal
  2nd:
  - type: bang
    description: restarts the object

outlets:
  1st:
  - type: signal
    description: maximum peak value so far in dB
  2nd:
  - type: bang
    description: bangs when input gets clipped

draft: false
---

[maxpeak~] returns the maximum peak amplitude so far in dB. A bang in the right inlet resets the maximum value. The right outlet sends bangs when the input is clipped (exceeds 0 dB).
