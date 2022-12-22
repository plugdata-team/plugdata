---
title: bonk~
description: attack detection and spectral envelope measurement
categories:
- object
see_also: {}
pdcategory: Extra
last_update: '0.42'
inlets:
  1st:
  - type: signal
    description: signal input to be analyzed.
  - type: thresh <float, float>
    description:  set low and high thresholds.
  - type: mask <float, float>
    description: "set energy mask profile: number of analysis and drop factor."
  - type: attack-frames <float>
    description: set number of frames over which to measure growth.
  - type: minlevel <float>
    description: set minimum "velocity" to output (ignore quieter notes).
  - type: spew <float>
    description: nonzero turns spew mode on, zero sets it off.
  - type: useloudness <float>
    description: nonzero sets to alternative loudness units (experimental).
  - type: bang
    description: poll the current spectrum via "raw" outlet.
  - type: debug <float>
    description: nonzero sets to debugging mode.
  - type: print <float>
    description: print out settings, templates and filterbank settings for nonzero.
  - type: debounce <float>
    description: set minimum time (in msec) between attacks in learn mode.
  - type: learn <float>
    description: forget and learn for a given number of times (10 recommended).
  - type: forget
    description: forget the last template.
  - type: write <symbol>
    description: write templates to a file.
  - type: read <symbol>
    description: read templates from a file.
outlets:
  1st:
  - type: list
    description: "raw: attack's spectrum (loudness values for the 11 frequency bands used)."
  2nd:
  - type: list
    description: "cooked: instrument number (if there's a template), velocity and temperature."
flags:	
- flag: -npts
  description: window size in points 
  default: 256
- flag: -hop
  description: analysis period ("hop size"