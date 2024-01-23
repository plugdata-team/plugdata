---
title: pix_record
description: write a sequence of pixes to a movie file
categories:
  - object
pdcategory: Gem, Graphics
methods:
    - type: file <symbol>
      description: specify the file for writing
    - type: record <float>
      description: start/stop recording
    - type: bang
      description: grab the next incoming pix
    - type: auto <float>
      description: start/stop grabbing all incoming pixes
    - type: codeclist
      description: codeclist: enumerate a list of available codecs to the outlet#3
    - type: codex <float>
      description: select codec
    - type: codec <symbol>
      description: select codec by short name
    - type: set <symbol> <float>
      description: set a property named ar1 to the value arg2
    - type: dialog
      description: popup a dialog to select the codec (if available)
inlets:
  1st:
    - type: gemlist
      description:
 
outlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: float
      description: number of frames written
  3rd:
    - type: list
      description: info on available codecs/properties
