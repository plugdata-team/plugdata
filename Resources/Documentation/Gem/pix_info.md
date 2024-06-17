---
title: pix_info
description: get information about the current image
categories:
  - object
pdcategory: Gem, Graphics
flags:
  - name: -m
    description: message mode
methods:
    - type: symbolic <float>
      description: output openGL constants as symbols rather than numbers when in message mode
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
      description: image-width (or in message mode, all info about image)
  3rd:
    - type: float
      description: image-height (unused in message mode)
  4th:
    - type: float
      description: bytes per pixel (unused in message mode)
  5th:
    - type: float
      description: format (unused in message mode)
  6th:
    - type: list
      description: <type> <upsidedown> <notowned> (unused in message mode)
  7th:
    - type: list
      description: <newimage> <newfilm> (unused in message mode)
  8th:
    - type: pointer
      description: image-data (unused in message mode)
draft: false
---
