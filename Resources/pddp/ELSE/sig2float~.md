---
title: sig2float~

description: Convert signal to float

categories:
 - object

pdcategory: General

arguments:
- type: float
  description: self-clocking interval in ms
  default: 0
- type: float
  description: sample offset within the block
  default: 0

flags:
- name: -off
  description: turns it off (default: on)

inlets:
  1st:
  - type: signal
    description: signal value to convert to float
  - type: bang
    description: syncs the conversion rate
  - type: set <f>
    description: sets the time without syncing
  - type: float
    description: turns on (non-0) or off (0)
  - type: start
    description: turns on
  - type: stop
    description: turns off
  - type: offset <f>
    description: sets the sample offset within the block
   2nd:
  - type: float
    description: conversion time in ms

outlets:
  1st:
  - type: float
    description: converted float

draft: false
---

[sig2float~] converts signals to floats. The conversion occurs at a time rate defined by the 1st argument or right inlet.