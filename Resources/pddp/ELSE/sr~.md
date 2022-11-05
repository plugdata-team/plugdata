---
title: sr~

description: Get/set sample rate

categories:
 - object

pdcategory: Math (Constant)

arguments: (none)

flags:
- name: -khz
  description: sets output to frequency in khz
- name: -ms
  description: sets output to period ms
- name: -sec
  description: sets output to period in seconds

inlets:
  1st:
  - type: bang
    description: get sample rate frequency or period
  - type: hz
    description: set and get the sample rate frequency in Hz
  - type: khz
    description: set and get the sample rate frequency in Khz
  - type: ms
    description: set and get the sample rate period in ms
  - type: sec
    description: set and get the sample rate period in seconds

outlets:
  1st:
  - type: float
    description: sample rate frequency or period

draft: false
---

[sr~] can set the sample rate and also reports the sample rate frequency or period when loading the patch, when receiving a bang or when it changes. The frequency is reported either in hz or khz and the period either in seconds os milliseconds. Unlike [samplerate~], it doesn't report up/down sampling rates. The default output is the sample rate frequency in hertz.
