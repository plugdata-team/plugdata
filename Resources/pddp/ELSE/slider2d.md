---
title: slider2d

description: Two-dimensional slider

categories:
- object

pdcategory: GUI

arguments: (none)

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
  description: non zero sets line visibility (default: 1)
- name: -grid <f>
  description: non zero sets grid visibility (default: 0)
- name: -bgcolor <f,f,f>
  description: sets background color (default: 255 255 255)
- name: -fgcolor <f,f,f>
  description: sets foreground color (default: 0 0 0)
- name: -init <f>
  description: non zero sets to init mode (default 0)


inlets:
  1st:
- type: bang
  description: outputs last values 
  default:
- type: list
  description: sets x, y and output it
  default:
- type: set <f,f>
  description: sets x and y
  default:
- type: size <f>
  description: sets size
  default:
- type: width <f>
  description: sets x (horizontal) size
  default:
- type: height <f>
  description: sets y (vertical) size
  default:
- type: xrange <f,f>
  description: sets x range
  default:
- type: yrange <f,f>
  description: sets y range
  default:
- type: line <f>
  description: - non zero sets line visibility
  default:
- type: grid <f>
  description: non zero sets grid visibility
  default:
- type: bgcolor <f,f,f>
  description: sets background color in RGB
  default:
- type: fgcolor <f,f,f>
  description: sets foreground color in RGB
  default:
- type: init <f>
  description: non zero sets to init mode
  default:

outlets:
  1st:
  - type: list
    description: x/y values

draft: false
---

[slider3d] is a two-dimensional slider GUI abstraction (see also [circle).
