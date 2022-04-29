---
title: fudiparse
description: FUDI messages to and from Pd lists
categories:
- object
pdcategory: I/O via MIDI, OSC, and FUDI
last_update: '0.48'
see_also:
- fudiformat
- oscformat
inlets:
  1st:
  - type: list
    description: FUDI packet to convert to Pd messages.
outlets:
  1st:
  - type: anything
    description: Pd messages.
draft: false
---
The fudiparse object takes incoming lists of numbers, interpreting them as the bytes in a FUDI message (as received when sending Pd-messages via [netreceive -b]).
