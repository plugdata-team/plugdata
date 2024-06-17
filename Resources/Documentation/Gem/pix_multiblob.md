---
title: pix_multiblob
description: blob detector (for multiple blobs)
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: max number N of blobs to detect
methods:
    - type: threshold <float>
      description: minimum luminance of a pixel to be considered part of a blob
    - type: blobSize <float>
      description: minimum relative size of a blob
inlets:
  1st:
    - type: gemlist
      description:
outlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: list
      description: matrix describing k detected blobs
draft: false
---
