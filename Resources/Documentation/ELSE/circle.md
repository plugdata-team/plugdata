---
title: circle

description: circular slider

categories:
- object

pdcategory: ELSE, UI

arguments:
inlets:
  1st:
  - type: bang
    description: outputs last values
  - type: list
    description: sets x, y and outputs it


outlets:
  1st:
  - type: list
    description: x/y values

methods:
  - type: set <float, float>
    description: sets x and y
  - type: size <float>
    description: sets size
  - type: range <float, float>
    description: sets range (minimum and maximum values of x and y)
  - type: yrange <float>
    description: sets range (minimum and maximum values of y)
  - type: xrange <float>
    description: sets range (minimum and maximum values of x)
  - type: line <float>
    description: non-0 sets line visibility
  - type: grid <float>
    description: non-0 sets grid visibility
  - type: bgcolor1 <f, f, f>
    description: sets background color inside circle in RGB
  - type: bgcolor2 <f, f, f>
    description: sets background color outside circle in RGB
  - type: bgcolor <f, f, f>
    description: sets both background colors in RGB
  - type: fgcolor <f, f, f>
    description: sets foreground color in RGB
  - type: savestate <float>
    description: non-0 will save object state
  - type: clip <float>
    description: non-0 clips inside the circle
    
flags:
  - name: -size <float>
    description: sets x/y size
    default: 127
  - name: -line <float>
    description: non-0 sets line visibility
    default: 1
  - name: -fgcolor <f, f, f>
    description: sets foreground color in RGB
    default: 0 0 0
  - name: -grid <float>
    description: non-0 sets grid visibility
    default: 0
  - name: -savestate <float>
    description: non-0 sets will save object state
    default: 0
  - name: -range <f, f>
    description: sets range (minimum and maximum values of x and y)
    default: -1 1
  - name: -xrange <f> / -yrange <f>
    description: set x/y range independently
  - name: -bgcolor1 <f, f, f>
    description: sets in bounds back color in RGB
    default: 255 255 255
  - name: -bgcolor2 <f, f, f>
    description: sets out of bounds color in RGB
    default: 255 255 255
  - name: -bgcolor <f, f, f>
    description: sets both background colors in RGB
    default: 255 255 255
  - name: -clip <float>
    description: non-0 sets to clip mode
    default: 1

draft: false
---

[circle] is a two dimensional slider GUI abstraction (see also [else/slider2d).
