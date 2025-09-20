---
title: scope3d~

description: 3D oscilloscope display

categories:
 - object

pdcategory: ELSE, UI, Analysis

inlets:
  1st:
  - type: signal
    description: signal to be displayed in the X axis
  - type: list
    description: one float sets 'nsamples' and 'nlines'
  2nd:
  - type: signal
    description: signal to be displayed on the Y axis
  3rd:
  - type: signal
    description: signal to be displayed on the Z axis

outlets:

flags:
  - name: -nsamples <float>
    description: sets number of samples per line (2-8192, default 256)
    default: 8
  - name: -nlines <float>
    description: sets number of lines in buffer (8-256, default 128)
    default: 256
  - name: -dim <f, f>
    description: sets width/height (default 200 100)
    default: 140 140
  - name: -fgcolor <f, f, f>
    description: sets front/signal RGB color (values 0-255)
  - name: -bgcolor <f, f, f>
    description: sets background RGB color (values 0-255)
  - name: -gridcolor <f, f, f>
    description: sets grid RGB color (values 0-255)
  - name: -zoom <float>
    description: sets zoom level
    default: 1
  - name: -perspective <float>
    description: sets perspective level
    default: 1
  - name: -rotate <f, f>
    description: rotate around x/y axis in degrees
  - name: -rotatex <float>
    description: rotate around x axis in degrees
  - name: -rotatey <float>
    description: rotate around y axis in degrees
  - name: -drag <float>
    description: enable/disable mouse interaction
    default: 1
  - name: -grid <float>
    description: enable/disable grid visualization
    default: 1
  - name: -rate <float>
    description: sets refresh rate period in ms
    default: 50
  - name: -width <float>
    description: sets line width
    default: 1

methods:
  - type: nsamples <float>
    description: sets number of samples per line (2-8192, default 256)
  - type: nlines <float>
    description: sets number of lines in buffer (8-256, default 128)
  - type: dim <f, f>
    description: sets width/height (default 200 100)
  - type: fgcolor <f, f, f>
    description: sets front/signal RGB color (values 0-255)
  - type: bgcolor <f, f, f>
    description: sets background RGB color (values 0-255)
  - type: gridcolor <f, f, f>
    description: sets grid RGB color (values 0-255)
  - type: zoom <float>
    description: sets zoom level
  - type: perspective <float>
    description: sets perspective level
  - type: rotate <f, f>
    description: rotate around x/y axis in degrees
  - type: rotatex <float>
    description: rotate around x axis in degrees
  - type: rotatey <float>
    description: rotate around y axis in degrees
  - type: receive <symbol>
    description: receive symbol (default empty)
  - type: drag <float>
    description: enable/disable mouse interaction
  - type: grid <float>
    description: enable/disable grid visualization
  - type: rate <float>
    description: sets refresh rate period in ms
  - type: width <float>
    description: sets line width
draft: false
---

[scope3d~] displays signals in 3D. This is an object coded in Lua and is very very experimental and likely to change and break.
