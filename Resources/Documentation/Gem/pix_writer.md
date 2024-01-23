---
title: pix_writer
description: write the current texture to a file
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
outlets:
  1st:
    - type: gemlist
      description:
draft: false
---
