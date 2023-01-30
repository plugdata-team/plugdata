---
title: graph~

description: signal graph

categories:
- object

pdcategory: ELSE, Analysis, Signal Math

arguments:

inlets:
  1st:
  - type: signal
    description: incoming signal to graph

outlets:
  1st:
  - type: signal
    description: input signal comes out here

flags:
  - name: -size <float>
    description: buffer size in samples (default 441, minimum 64)
  - name: -skip <float>
    description: buffer skip (default 10)
  - name: bgcolor <f, f, f>
    description: background color in RGB (default 223 223 223)
  - name: fgcolor <f, f, f>
    description: foreground color in RGB (default 0 0 0)
  - name: width <float>
    description: set signal width (default 1)
  - name: -range <float, float>
    description: set lower and upper graph range (default -1 1)
  - name: -dim <float, float>
    description: set horizontal/vertical dimensions (default 200 140)

methods:
  - type: size <float>
    description: buffer size in samples
  - type: skip <float>
    description: number of buffers to skip
  - type: bgcolor <f, f, f>
    description: background color in RGB
  - type: fgcolor <f, f, f>
    description: foreground color in RGB
  - type: width <float>
    description: set signal width
  - type: range <float, float>
    description: set min/max range
  - type: dim <float, float>
    description: set horizontal/vertical dimensions

draft: false
---

[graph~] is a very simple but convenient abstraction for visualizing audio signals.

