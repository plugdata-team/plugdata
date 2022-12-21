---
title: nyquist~
description: Get nyquist
categories:
 - object
pdcategory: General
arguments:
- type: none
  description:
  default:

inlets:
  1st:
  - type: bang
    description: get nyquist frequency or period

outlets:
  1st:
  - type: float
    description: nyquist frequency or period

flags:
  - name: -khz
    description: sets to frequency in khz
  - name: -ms
    description: sets to period in ms
  - name: -sec
    description: sets to period in seconds

methods:
  - type: hz
    description: set and get the nyquist frequency in Hz
  - type: khz
    description: set and get the nyquist frequency in Khz
  - type: ms
    description: set and get the nyquist period in ms
  - type: sec
    description: set and get the nyquist period in seconds

---

[nyquist~] reports the nyquist (which is half the sample rate) as a frequency or period. It sends it when loading the patch, when receiving a bang or when the sample rate changes. It reports it either in hz or khz and the period either in seconds os milliseconds. like [sr~], it doesn't report up/down sampling rates.
