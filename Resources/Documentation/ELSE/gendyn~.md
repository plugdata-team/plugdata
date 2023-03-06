---
title: gendyn~
description: dynamic stochastic synthesis

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:

inlets:

outlets:
  1st:
  - type: signal
    description: dynamic stochastic synthesis output

flags:
  - name: -n <f>
  - name: -frange <f f>
  - name: -freq_step <f>
  - name: -amp_step <f>
  - name: -interp <f>
  - name: -seed <f>
  - name: -dist <f f f f>

methods:
  - type: n <float>
    description: set number of points (default 12)
  - type: frange <f, f>
    description: set minimum and maximum frequencies (default 220, 440)
  - type: freq_step <float>
    description: set maximum frequency step in % (default 50)
  - type: amp_step <float>
    description: set maximum amplitude step in % (default 25)
  - type: interp <float>
    description: sets interpolation mode (default 1, linear)
  - type: seed <float>
    description: sets seed, no float sets unique internal (default)
  - type: dist <f, f, f, f>
    description: sets distributions and parameters, see [pd distributions]

draft: false
---

[gendyn~] implements "Dynamic Stochastic Synthesis" based on Xenakis' 'GenDyn' algorithm. The waveform has a given number of points (linearly interpolated by default). The period of each point and its value changes according to a random walk (a.k.a. Brownian motion) with given distributions.

