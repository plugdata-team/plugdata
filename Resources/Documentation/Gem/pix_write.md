---
title: pix_write
description: Make a snapshot of the frame-buffer and write it to a file
categories:
  - object
pdcategory: Gem, Graphics

methods:
  - name: file <float>
    description: set type/quality (0 = TIFF, >0 = JPG)
  - name: file <symbol>
    description: set basefilename, and type = TIFF
  - name: file <symbol> <float>
    description: set basefilename and type/quality
inlets:
    1st:
      - type: gemlist
        description:
    2nd:
      - type: list
        description: x and y offset
    3rd:
      - type: list
        description: width and height
outlets:
  1st:
    - type: gemlist
draft: false
---
