---
title: circle

description: Circular slider

categories:
- object

pdcategory: GUI

arguments: none

inlets:
  1st:
  - type: bang
    description: outputs last values
  - type: list
    description: sets x, y and outputs it
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
    description: non zero sets line visibility
  - type: grid <float>
    description: non zero sets grid visibility
  - type: bgcolor1 <f, f, f>
    description: sets background color inside circle in RGB
  - type: bgcolor2 <f, f, f>
    description: sets background color outside circle in RGB
  - type: bgcolor <f, f, f>
    description: sets both background colors in RGB
  - type: fgcolor <f, f, f>
    description: sets foreground color in RGB
  - type: init <float>
    description: non zero sets to init mode
  - type: clip <float>
    description: non zero clips inside the circle

outlets:
  1st:
  - type: list
    description: x/y values

flags:
  - name:
  description: (see pd flags)

draft: false
---

[circle] is a two dimensional slider GUI abstraction (see also [else/slider2d).