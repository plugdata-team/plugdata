---
title: vsnapshot~
description: deluxe snapshot~
categories:
- object
see_also:
- snapshot~
pdcategory: General Audio Manipulation
last_update: '0.47'
inlets:
  1st:
  - type: bang
    description: convert a signal to a float.
outlets:
  1st:
  - type: float
    description: the converted signal at every bang.
draft: false
---
mal-designed snapshot~ extension, best not to use this.

This is an attempt at making a version of snapshot~ that trades off delay for time jitter control. The behaviour is the same as snapshot~ (it takes a bang and converts a signal to a float. The idea is that you can convert from an audio signal and get a specific value within an audio block depending on the exact time it receives a bang. Unfortunately it isn't fully correct and will be replaced by a more correct one in the future. Since this change will probably be incompatible with this object, it is probably best to avoid using it until it is working correctly.