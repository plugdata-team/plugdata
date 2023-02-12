---
title: sfont~

description: soundfont synthesizer

categories:
 - object

pdcategory: ELSE, MIDI

arguments:
- type: symbol
  description: soundfont file to load
  default: none

flags:
- name: -v
  description: set to verbosity mode
- name: -ch <float>
  description: set the number of channels (default 16)
- name: -g <float>
  description: set the gain, 0-1 (default 0.4)

inlets:
  1st:
  - type: float
    description: raw MIDI input

outlets:
  1st:
  - type: signal
    description: left output signal of stereo output
  2nd:
    - type: anything
        description: right output signal of stereo output
  3rd:
    - type: anything
      description: preset and scale name

methods:
  - type: open <symbol>
    description: loads soundfont file (.sf2/.sf3 extensions implied)
  - type: note <f, f, f>
    description: key, velocity, channel
  - type: list
    description: same as note
  - type: ctl <f, f, f>
    description: controller number, value, channel
  - type: bend <f, f>
    description: pitch bend value, channel
  - type: touch <f, f,>
    description: value, channel
  - type: polytouch <f, f, f>
    description: key, value, channel
  - type: pgm <f, f>
    description: program number, channel
  - type: bank <f, f>
    description: bank number, channel
  - type: transp <f, f>
    description: cents, channel
  - type: pan <f, f>
    description: pan value, channel
  - type: sysex <list>
    description: sysex message
  - type: panic
    description: resets synth and clears hanging notes
  - type: version
    description: prints version info on terminal
  - type: info
    description: prints soundfont info on terminal
  - type: verbose <float>
    description: non-0 sets verbose mode
  - type: remap <list>
    description: list of 128 pitches in MIDI remaps all keys
  - type: scale <list>
    description: scale in cents to retune (12-tone temperament if no list)
  - type: base <float>
    description: base MIDI pitch
  - type: set-tuning <list>
    description: sets tuning <bank, program, channel & name> for a scale
  - type: sel-tuning <list>
    description: selects tuning <bank, program, channel> for a scale
  - type: unsel-tuning <list>
    description: unselects tuning <bank, program, channel> for a scale

draft: false
---

[sfont~] is a sampler synthesizer that plays SoundFont files. It is based on FluidSynth.
