---
title: sfz~

description: hybrid sample player

categories:
 - object
 
pdcategory: ELSE, MIDI

arguments:
- type: symbol
  description: sets file to load
  default: none

inlets:
  1st:
  - type: float
    description: raw MIDI input
  - type: list
    description: key, velocity

outlets:
  1st:
  - type: signal
    description: left output
  2nd:
  - type: signal
    description: right output

methods:
  - type: open <float>
    description: load soundfont file (.sfz extension implied)
  - type: note
    description: key, velocity
  - type: ctl <float>
    description: control change: value, control
  - type: bend <float>
    description: pitch bend value
  - type: touch <float>
    description: aftertouch value
  - type: polytouch <f, f>
    description: polytouch value: value, key
  - type: flush
    description: sends note offs for all notes
  - type: panic
    description: stops all sound immediately
  - type: scale <list>
    description: scale in cents to retune (12-tone temperament if no list is given)
  - type: base <float>
    description: base MIDI pitch for scale
  - type: scala <symbol>
    description: open scala tuning file
  - type: a4 <float>
    description: sets frequency of A4 in hertz
  - type: transp <float>
    description: transposition: cents, channel (optional)

draft: false
---

[sfz~] is based on "sfizz" and is a synthesizer for soundfont instruments in the SFZ format. For more details on the SFZ format, players and instruments for download, see https://sfzformat.com/