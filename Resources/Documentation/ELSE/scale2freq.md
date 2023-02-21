---
title: scale2freq

description: convert scale into a frequency list

categories:
 - object

pdcategory: ELSE, Tuning, Data Math, Converters

arguments:
  - type: list
    description: scale in cents

inlets:
  1st:
  - type: list
    description: scale in cents to convert to a frequency list
  - type: bang
    description: generate frequency list

outlets:
  1st:
  - type: list
    description: frequency list

flags:
  - name: -base <float>
    description: sets base pitch value in MIDI (default 60)
  - name: -range <float float>
    description: sets min/max frequency range in Hz (default 20 20000)

methods:
  - type: base <float>
    description: sets base pitch value in MIDI
  - type: range <f, f>
    description: sets min/max frequency range in Hz

draft: false
---

[scale2freq] gets a scale as a list of cents values, a base/fundamental pitch and outputs a list of frequency in Hz between a minimum and maximum value. Below we use [eqdiv] to generate a scale. Use it to feed values to things like [resonbank~], [oscbank2~] or [pvretune~].

