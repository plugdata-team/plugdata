---
title: slider2d

description: two-dimensional slider

categories:
- object

pdcategory: ELSE, UI

arguments:

flags:
- name: -size <f>
  description:  sets x/y size (default: 127)
- name: -dim <f,f>
  description: sets x/y dimensions independently (default: 127)
- name: -range
  description: sets x/y range (default: 0 127)
- name: -xrange/-yrange
  description: sets x/y range independently
- name: -line <f>
  description: non-0 sets line visibility (default: 1)
- name: -grid <f>
  description: non-0 sets grid visibility (default: 0)
- name: -bgcolor <f,f,f>
  description: sets background color (default: 255 255 255)
- name: -fgcolor <f,f,f>
  description: sets foreground color (default: 0 0 0)
- name: -init <f>
  description: non-0 sets to init mode (default 0)


inlets:
  1st:
- type: bang
  description: outputs last values 
- type: list
  description: sets x, y and output it

outlets:
  1st:
  - type: list
    description: x/y values

methods:
- type: set <f,f>
  description: sets x and y
- type: size <f>
  description: sets size
- type: width <f>
  description: sets x (horizontal) size
- type: height <f>
  description: sets y (vertical) size
- type: xrange <f,f>
  description: sets x range
- type: yrange <f,f>
  description: sets y range
- type: line <f>
  description: - non-0 sets line visibility
- type: grid <f>
  description: non-0 sets grid visibility
- type: bgcolor <f,f,f>
  description: sets background color in RGB
- type: fgcolor <f,f,f>
  description: sets foreground color in RGB
- type: init <f>
  description: non-0 sets to init mode

draft: false
---

[slider3d] is a two-dimensional slider GUI abstraction (see also [circle).
