---
title: pix_curve
description: applies color-curves stored in arrays to an image
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: list
    description: table name per channel
methods:
  - type: set <symbol>
    description: common table for all channels
  - type: set <symbol> <symbol> <symbol>
    description: separate tables for all channels
  - type: set <symbol> <symbol> <symbol> <symbol>
    description: separate tables for all channels (incl. alpha)
inlets:
  1st:
    - type: gemlist
      description:
outlets:
  1st:
    - type: gemlist
draft: false
---
