---
title: bonk~
description: attack detection and spectral envelope measurement
categories:
- object
see_also: {}
pdcategory: vanilla, Analysis
last_update: '0.42'
inlets:
  1st:
  - type: signal
    description: signal input to be analyzed
  - type: bang
    description: poll the current spectrum via "raw" outlet
outlets:
  1st:
  - type: list
    description: "raw" attack's spectrum (loudness values for the 11 frequency bands used)
  2nd:
  - type: list
    description: "cooked" instrument number (if there's a template), velocity and temperature
flags:	
- name: -npts
  description: window size in points 
  default: 256
- name: -hop
  description: analysis period ("hop size") in points (default 128)
- name: -nfilters
  description: number of filters to use (default 11)
- name: -halftones
  description: filter bandwidth of filters in halftones (default 6)
- name: -minbandwidth
  description: minimum bandwidth in bins (default 1.5)
- name: -overlap
  description: overlap factor between filters (default 1)
- name: -firstbin
  description: center frequency, in bins, of the lowest filter (default 1)

methods:
  - type: thresh <float, float>
    description:  set low and high thresholds
  - type: mask <float, float>
    description: set energy mask profile: number of analysis and drop factor
  - type: attack-frames <float>
    description: set number of frames over which to measure growth
  - type: minlevel <float>
    description: set minimum "velocity" to output (ignore quieter notes)
  - type: spew <float>
    description: non-0 turns spew mode on, zero sets it off
  - type: useloudness <float>
    description: non-0 sets to alternative loudness units (experimental)
  - type: debug <float>
    description: non-0 sets to debugging mode
  - type: print <float>
    description: print out settings, templates and filter bank settings for non-0
  - type: debounce <float>
    description: set minimum time (in msec) between attacks in learn mode
  - type: learn <float>
    description: forget and learn for a given number of times (10 recommended)
  - type: forget
    description: forget the last template
  - type: write <symbol>
    description: write templates to a file
  - type: read <symbol>
    description: read templates from a file

draft: false
---
attack detection and spectral envelope measurement

