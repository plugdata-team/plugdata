---
title: sigmund~
description: sinusoidal analysis and pitch tracking
categories:
- object
see_also: {}
pdcategory: "'EXTRA' (patches and externs in pd/extra)"
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
  description: set number of points in each analysis window (default 1024).
- flag: -hop <float>
  description: set number of points between each analysis (default 512).
- flag: -npeak <float>
  description: set number of sinusoidal peaks for 'peaks' output (default 20).
- flag: -maxfreq <float>
  description: set maximum sinusoid frequency in Hz (default 1000000).
- flag: -vibrato <float>
  description: set depth of vibrato to expect in semitones (default 1).
- flag: -stabletime <float>
  description: set time in msec to wait to report new notes (default 50).
- flag: -minpower <float>
  description: set minimum amplitude in dB to report a new note (default 50).
- flag: -growth <float>
  description: set amplitude growth in dB to report a repeated note (default 7).
arguments:
- type: list
  description: "sets outlets and types: pitch, notes, env, peaks, tracks (default: 'pitch env')."
draft: false
---
Sigmund~ analyzes an incoming sound into sinusoidal components, which may be reported individually or combined to form a pitch estimate. Possible outputs are specified as creation arguments:

- pitch - output pitch continuously
- notes - output pitch at the beginning of notes
- env - output amplitude continuously
- peaks - output all sinusoidal peaks in order of amplitude
- tracks - output sinusoidal peaks organized into tracks

Parameters you may set in creation arguments as flags or messages:

- npts - number of points in each analysis window (1024)
- hop - number of points between each analysis (512)
- npeak - number of sinusoidal peaks (20)
- maxfreq - maximum sinusoid frequency in Hz. (1000000)

(. the parameters below affect note detection, not raw pitch .)

- vibrato - depth of vibrato to expect in 1/2-tones (1)
- stabletime - time (msec) to wait to report notes (50)
- minpower - minimum power (dB) to report a note (50)
- growth - growth (dB) to report a repeated note (7)

The npts and hop parameters are in samples, and are powers of two. The example below specifies a huge hop of 4096 (to slow the output down) and to output "pitch" and "env" (the default outputs).