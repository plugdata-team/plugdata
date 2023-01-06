---
title: sigmund~
description: sinusoidal analysis and pitch tracking
categories:
- object
see_also: {}
pdcategory: Extra
last_update: '0.46'
inlets:
  1st:
  - type: signal
    description: signal input to be analyzed.
  - type: list
    description: "analyze an array (with the '-t' flag): table name, number of points, index of first point, sample rate and debug flag."
  - type: npts <float>
    description: set number of points in each analysis window)
  - type: hop <float>
    description: set number of points between each analysis.
  - type: npeak <float>
    description: set number of sinusoidal peaks for 'peaks' output.
  - type: maxfreq <float>
    description: set maximum sinusoid frequency in Hz.
  - type: vibrato <float>
    description: set depth of vibrato to expect in semitones (for note output).
  - type: stabletime <float>
    description: set time in msec to wait to report new notes (note output).
  - type: minpower <float>
    description: set minimum amplitude in dB to report a new note (note output).
  - type: growth <float>
    description: set amplitude growth in dB to report a repeated note (note output).
outlets:
  (number of outlets, order and type depend on creation arguments):
  pitch:
  - type: float
    description: pitch in MIDI.
  notes:
  - type: float
    description: pitch in MIDI at the beginning of notes.
  env:
  - type: float
    description: amplitude in dB.
  peaks:
  - type: list
    description: partial index, frequency, amplitude, cosine and sine components.
  tracks:
  - type: list
    description: partial index, frequency, amplitude and flag.
flags:
- flag: -t
  description: analyzes waveforms stored in arrays.
- flag: -npts <float>
  description: set number of points in each analysis window 
  default: 1024
- flag: -hop <float>
  description: set number of points between each analysis 