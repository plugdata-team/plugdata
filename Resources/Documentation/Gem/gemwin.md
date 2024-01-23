---
title: gemwin

description: interact with Gem window

categories:
 - graphics

pdcategory: Gem, Graphics

arguments:
- type: float
  description: frames per second
  default: 20
- type: symbol
  description: context name
  default: 20

methods:
- type: create
  description: create Gem window
- type: destroy
  description: remove Gem window
- type: reset
  description: reset Gem window attributes
- type: dimen <float> <float>
  description: set Gem window size
- type: offset <float> <float>
  description: set Gem window position
- type: FSAA <float>
  description: enable anti-aliasing
- type: frame <float>
  description: set frames per second
- type: color <float> <float> <float>
  description: set background colour
- type: print
  description: print information

inlets:
  1st:
  - type: float
    description: start/stop rendering

outlets:

draft: false
---