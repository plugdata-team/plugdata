---
title: scope~

description: oscilloscope display

categories:
 - object

pdcategory: ELSE, UI, Analysis

inlets:
  1st:
  - type: signal
    description: signal to be displayed in the X axis
  - type: list
    description: one float sets 'nsamples', optional second sets 'nlines'
  2nd:
  - type: signal
    description: signal to be displayed on the Y axis

outlets:

flags:
  - name: -fgcolor <f f f>
  - name: -bgcolor <f f f>
  - name: -gridcolor <f f f>
  - name: -range <f f>
  - name: -trigger <f>
  - name: -triglevel <f>
  - name: -nsamples <f>
  - name: -nlines <f>
  - name: -delay <f>
  - name: -drawstyle <f>
  - name: -dim <f f>
  - name: -receive <sym>

methods:
  - type: nsamples <float>
    description: sets number of samples per line (2-8192, default 256)
  - type: nlines <float>
    description: sets number of lines in buffer (8-256, default 128)
  - type: dim <f, f>
    description: sets width/height (default 200 100)
  - type: range <f, f>
    description: sets vertical range (default -1 1)
  - type: fgcolor <f, f, f>
    description: sets front/signal RGB color (values 0-255)
  - type: bgcolor <f, f, f>
    description: sets background RGB color (values 0-255)
  - type: gridcolor <f, f, f>
    description: sets grid RGB color (values 0-255)
  - type: drawstyle <float>
    description: <1> sets alternate drawing style (default 0)
  - type: trigger <float>
    description: sets trigger mode: 0 (none, default), 1 (up) or 2 (down)
  - type: triglevel <float>
    description: sets threshold level for the trigger mode (default 0)
  - type: delay <float>
    description: onset time delay between displays (default 0)
  - type: receive <symbol>
    description: receive symbol (default empty)

draft: false
---

[oscope~] displays a signal in the style of an oscilloscope.
