---
title: spectrograph~

description: specrtal graph

categories:
- object

pdcategory: ELSE, UI, Analysis

arguments:

flags:
- name: -size <f>
  description: FFT size (default 1024, min 128)
- name: -db <f>
  description: non-0 sets to dB amp scale (default linear)
- name: -log <f>
  description: non-0 sets to log frequency scale (default linear)
- name: -dim <f,f>
  description: set horizontal/vertical dimensions (default 300 140)
- name: -rate <float>
  description: sets graph rate in ms (default 100)

inlets:
  1st:
  - type: signal
    description: incoming signal to graph
  - type: bang
    description: graphs when rate is 0

methods:
  - type: size <f>
    description: sets FFT size
  - type: db <f>
    description: non-0 sets to dB amp scale
  - type: log <f>
    description: non-0 sets to log frequency scale
  - type: dim <f,f>
    description: set horizontal/vertical dimensions
  - type: rate <f>
    description: sets graph rate in ms


draft: false
---

[spectrograph~] is an abstraction for visualizing FFT amplitudes from 0Hz to Nyquist. It uses a Hann table for the analysis.
