---
title: bonk~
description: attack detection and spectral envelope measurement
categories:
- object
see_also: {}
pdcategory: "'EXTRA' (patches and externs in pd/extra)"
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
  description: window size in points (default 256).
- flag: -hop
  description: analysis period ("hop size") in points (default 128).
- flag: -nfilters
  description: number of filters to use (default 11).
- flag: -halftones
  description: filter bandwidth of filters in halftones (default 6).
- flag: -minbandwidth
  description: minimum bandwidth in bins (default 1.5).
- flag: -overlap
  description: overlap factor between filters (default 1)
- flag: -firstbin
  description: center frequency, in bins, of the lowest filter (default 1).
draft: false
---
The Bonk object takes an audio signal input and looks for "attacks" defined as sharp changes in the spectral envelope of the incoming sound. Optionally, and less reliably, you can have Bonk check the attack against a collection of stored templates to try to guess which of two or more instruments was hit. Bonk is described theoretically in the 1998 ICMC proceedings, reprinted on msp.ucsd.edu.

Bonk's two outputs are the raw spectrum of the attack (provided as a list of 11 numbers giving the signal "loudness" in the 11 frequency bands used), and the "cooked" output which gives an instrument number (counting up from zero), "velocity" and "color temperature". The instrument number is significant only if bonk~ has a "template set" in memory. The "velocity" is the sum of the square roots of the amplitudes of the bands (normalized so that 100 is an attack of amplitude of about 1). The "temperature" is a sort of 'spectral centroid' that correlates with perceived brilliance.

By default bonk's analysis is carried out on a 256-point window (6 msec at 44.1 kHz) and the analysis period is 128 samples. These and other parameters may be overridden using creation arguments.