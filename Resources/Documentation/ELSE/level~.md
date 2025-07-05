---
title: level~

description: mono or multichannel dB level adjustment

categories:
- object

pdcategory: ELSE, Data Management

flags:
  - type: -savestate
    description: sets to state saving mode

methods:
  - type: set <float>
    description: set dB value
  - type: savestate <float>
    description: set dB value

inlets:
  1st:
  - type: signals
    description: incoming signal(s) whose gain will be adjusted

outlets:
  1st:
  - type: signals
    description: the adjusted gain signal

draft: false
---

[level~] is a convenient abstraction for adjusting a signal's gain in dB. At "12 o'clock" we have 'Unity Gain", which is 0 dB. Moving clockwise we linearly up to 12dB, whereas counterclockwise moves nonlinearly to -100, and reaching '-100' effectively means -inf dB (total silence). It has support for multichannel signals.
