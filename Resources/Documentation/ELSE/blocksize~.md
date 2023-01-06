---
title: blocksize~

description: Get block size

categories:
 - object

pdcategory: Accessing Data

arguments: none

inlets:
  1st:
  - type: bang
    description: get block size period or frequency
  - type: samps
    description: set and get the block size period in samples
  - type: ms
    description: set and get the sample rate period in ms
  - type: hz
    description: set and get the block size frequency in Hz

outlets:
  1st:
  - type: float
    description: block size period or frequency

flags:
- name: -hz
  description: sets to frequency in Hz
- name: -ms
  description: sets to period in ms

draft: false
---

[blocksize~] reports the block size when the audio is turned on, when it changes or via a bang. The period is reported in samples or ms, but you can also query the corresponding frequency in hz. The default output is the block size in samples. The DSP needs to be turned on so a block size is detected!