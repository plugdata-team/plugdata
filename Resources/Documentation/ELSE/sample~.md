---
title: sample~

description: audio buffer

categories:
- object

pdcategory: ELSE, Buffers

arguments:
- type: symbol
  description: buffer name 
  default: default sample_$0
- type: symbol
  description: audio file to load (optional)
  default: none

flags:
- name: -size
  description: maximum buffer size in samples
- name: -ms
  description: maximum buffer size in ms
- name: -ch
  description: number of channels (default - 1)

inlets:
  1st:
  - type: open <symbol>
    description: opens sample file
  - type: info
    description: output size, sample rate and channels
  - type: reopen
    description: reopens the previously opened file
  - type: clear
    description: fills all arrays with zeros
  - type: trim <f,f>
    description: trims to a sample range
  - type: save <symbol>
    description: sets file to write to
  - type: depth <f>
    description: bit depth (16, 24 or 32)
  - type: show
    description: opens buffer supbatch window
  - type: hide
    description: closes buffer supbatch window
  - type: sr <f>
    description: sets sample rate (default - patch's)
  - type: rename <symbol>
    description: renames buffer
  - type: normalize <f>
    description: - normalizes to the given float value
  - type: size <f>
    description: maximum buffer size in samples
  - type: ms <f>
    description: maximum buffer size in ms

outlets:
  1st:
  - type: list <f,f,f>
    description: sample's size, sample rate and channels

draft: false
---

[sample~] is an abstraction that creates an audio buffer which you can use to load a sample or record into. It can load and save multichannel files.
