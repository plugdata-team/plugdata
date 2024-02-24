---
title: pix_histo
description: gets the histogram (density function) of an image.
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: <list>
    description: tables to store histogram
methods:
    - type: set <symbol>
      description: set table to store grey-histogram
    - type: set <symbol> <symbol> <symbol>
      description: set tables to store RGB-histograms
    - type: set <symbol> <symbol> <symbol> <symbol>
      description: set tables to store RGBA-histograms
inlets:
  1st:
    - type: gemlist
      description:
outlets:
  1st:
    - type: gemlist
draft: false
---
