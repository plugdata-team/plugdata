---
title: nyquist~

description: get Nyquist

categories:
 - object

pdcategory: ELSE, Analysis

arguments:

inlets:
  1st:
  - type: bang
    description: get Nyquist frequency or period

outlets:
  1st:
  - type: float
    description: Nyquist frequency or period

flags:
  - name: -khz
    description: sets to frequency in kHz
  - name: -ms
    description: sets to period in ms
  - name: -sec
    description: sets to period in seconds

methods:
  - type: hz
    description: set and get the Nyquist frequency in Hz
  - type: khz
    description: set and get the Nyquist frequency in kHz
  - type: ms
    description: set and get the Nyquist period in ms
  - type: sec
    description: set and get the Nyquist period in seconds

draft: false
---

[nyquist~] reports the Nyquist (which is half the sample rate) as a frequency or period. It sends it when loading the patch, when receiving a bang or when the sample rate changes. It reports it either in Hz or kHz and the period either in seconds or milliseconds. like [sr~], it doesn't report up/down sampling rates.
