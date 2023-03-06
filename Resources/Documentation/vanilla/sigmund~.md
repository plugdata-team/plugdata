---
title: sigmund~
description: sinusoidal analysis and pitch tracking
categories:
- object
see_also:
pdcategory: vanilla, Analysis
last_update: '0.46'
arguments:
- type: list
  description: sets outlets and types: pitch, notes, env, peaks, tracks
  default: "pitch env"
inlets:
  1st:
  - type: signal
    description: signal input to be analyzed
  - type: list
    description: (with '-t' flag) table name, num of points, index of first point, sample rate, and debug flag
outlets:
  pitch:
  - type: float
    description: pitch in MIDI
  notes:
  - type: float
    description: pitch in MIDI at the beginning of notes
  env:
  - type: float
    description: amplitude in dB
  peaks:
  - type: list
    description: partial index, frequency, amplitude, cosine and sine components
  tracks:
  - type: list
    description: partial index, frequency, amplitude and flag
flags:
- name: -t
  description: analyzes waveforms stored in arrays
- name: -npts <float>
  description: set number of points in each analysis window (default 1024)
- name: -hop <float>
  description: set number of points between each analysis (default 512)
- name: -npeak <float>
  description: set number of sinusoidal peaks for 'peaks' output (default 20)
- name: -maxfreq <float>
  description: set maximum sinusoid frequency in Hz (default 1000000)
- name: -vibrato <float>
  description: set depth of vibrato to expect in semitones (default 1)
- name: -stabletime <float>
  description: set time in msec to wait to report new notes (default 50)
- name: -minpower <float>
  description: set minimum amplitude in dB to report a new note (default 50)
- name: -growth <float>
  description: set amplitude growth in dB to report a repeated note (default 7)

methods:
  - type: npts <float>
    description: set number of points in each analysis window
  - type: hop <float>
    description: set number of points between each analysis
  - type: npeak <float>
    description: set number of sinusoidal peaks for 'peaks' output
  - type: maxfreq <float>
    description: set maximum sinusoid frequency in Hz
  - type: vibrato <float>
    description: set depth of vibrato to expect in semitones (for note output)
  - type: stabletime <float>
    description: set time in msec to wait to report new notes (note output)
  - type: minpower <float>
    description: set minimum amplitude in dB to report a new note (note output)
  - type: growth <float>
    description: set amplitude growth in dB to report a repeated note (note output)

draft: true #outlets ordering
---
