---
title: record~

description: record signals in arrays

categories:
 - object

pdcategory: cyclone, Arrays and Tables

arguments:
- type: symbol
  description: array name (required)
  default:
- type: float
  description: channels to record (1-64)
  default: 1

inlets:
  1st:
  - type: signal
    description: signal to record into array 
  - type: float
    description: start (non-0), stop (0) recording
  - type: list
    description: loop start and end points in ms
  2nd:
  - type: float/signal
    description: start point in ms
  3rd:
  - type: float/signal
    description: end point in ms

outlets:
  1st:
  - type: signal
    description: sync output: signal from 0 (at start point) to 1 (at end point)

flags:
  - name: @append <float>
    description: append mode on (1) or off (0)
    default: 0
  - name: @loop <float>
    description: loop mode on (1) or off (0)
    default: 0
  - name: @loopstart <float>
    description: loop start point in ms
    default: 0
  - name: @loopend <float>
    description: loop end point in ms
    default: 0
  

methods:
  - type: set <symbol>
    description: set array for recording signals
  - type: reset
    description: resets loop points and restarts recording
  - type: append <float>
    description: append mode on (1) or off (0)
  - type: loop <float>
    description: loop mode on (1) or off (0)
  - type: loopstart <float>
    description: loop start point in ms
  - type: loopend <float>
    description: loop end point in ms

draft: true
---

[record~] records up to 64 signal channels into arrays. By default, recording stops when the array is filled.
