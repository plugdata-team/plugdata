---
title: bicoeff

description: biquad filter coefficient generator GUI

categories:
 - object

pdcategory: ELSE, Filters, Data Math

arguments:
inlets:
  1st:
  - type: anything
    description: <allpass, lowpass, highpass, bandpass, resonant, bandstop, eq, lowshelf, highshelf>

outlets:
  1st:
  - type: list
    description: 5 coefficients for the vanilla [biquad~] object

flags:
  - name: -dim <f,f>
    description: width, height (default: 450, 150)
  - name: -type <fsymbol>
    description: filter type (default: eq)

draft: false
---

[bicoeff] is a GUI that generates coefficients for vanilla's [biquad~] according to different filter types ("eq" filter by default as in the example below). Click on it and drag it around.
