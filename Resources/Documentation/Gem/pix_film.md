---
title: pix_film
description: load in a movie-file
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: symbol
    description: file to load initially
methods:
  - type: open <symbol> <symbol>
    description: opens the movie arg1, decodes it into the specified arg2 color-space if supported
  - type: colorspace <symbol>
    description: RGBA, YUV or Grey
  - type: auto <float>
    description: starts/stops automatic playback. (default:0)
  - type: loader <symbol>
    description: open the film using only the specified backend(s)
inlets:
  1st:
    - type: gemlist
      description:
    - type: bang
      description: (re)send the l/w/h/fps info to the 2nd outlet
  2nd:
    - type: float
      description: changes the frame to be decoded on rendering
outlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: list
      description: length, width, height and fps
  3rd:
    - type: bang
      description: indicates that the last frame has been reached
draft: false
---
