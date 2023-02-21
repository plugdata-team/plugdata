---
title: pd~
description: run a pd sub-process
categories:
- object
see_also:
- stdout
pdcategory: vanilla, UI
last_update: '0.42'
inlets:
  1st:
  - type: anything
    description: first element names an object, rest is sent to that object in the sub-process
  - type: signal
    description: signal input if there's a corresponding adc~ input
  nth:
  - type: signal
    description: signal input if there's a corresponding adc~ input
outlets:
  1st:
  - type: anything
    description: messages from sub-process sent via "stdout" objects
  nth:
  - type: signal
    description: signal output if there's a corresponding dac~ output
flags:
- name: -ninsig <float>
  description: sets number of input audio channels (default 2)
- name: -noutsig <float>
  description: sets number of output audio channels (default 2)
- name: -sr <float>
  description: sets sample rate of subprocess (default pd's current)
- name: -fifo <float>
  description: sets number of blocks for round-trip (default 5)
- name: -pddir <symbol>
  description: sets Pd's directory (needed if different than default)
- name: -scheddir <symbol>
  description: sets scheduler's directory (also needed if different)

methods:
  - type: pd~ start <anything>
    description: start a new sub-process. This message takes startup flags and needs a pd file to open
  - type: pd~ stop
    description: stops the pd sub-process
draft: false
---
