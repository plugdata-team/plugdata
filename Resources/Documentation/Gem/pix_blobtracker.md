---
title: pix_blobtracker
description: blob detector and tracker
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: max number of blobs to detect
methods:
  - type: threshold <float>
    description: minimum luminance of a pixel to be considered part of a blob
  - type: blobSize <float>
    description: minimum relative size of a blob
inlets:
  1st:
    - type: gemlist
outlets:
  1st:
    - type: gemlist
draft: false
---
