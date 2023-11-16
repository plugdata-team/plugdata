---
title: pm2~

description: 2 operator phase modulation matrix

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:
- type: float
  description: frequency in hz
  default: 0

inlets:
  1st:
  - description: frequency in hertz
    type: float/signal
  2nd:
  - description: master level for operator 1
    type: signal
  3rd:
  - description: master level for operator 2
    type: signal

outlets:
  1st:
  - description: left output
    type: signal
  2nd:
  - description: right output
    type: signal

flags:
  - name: -ratio <float>
    description:  set frequency ratio (default all = 1)
  - name: -vol <list>
    description: set volumes (default all = 1)
  - name: -detune <list>
    description: set frequency detuning (default all = 0)
  - name: -pan <list>
    description: set paning (default all = 0)
  - name: -idx <list>
    description: set modulation matrix (default all = 0)

methods:
  - type: idx <list>
    description: set modulation matrix values
  - type: 'i'to'j' <float>
    description: set index ('1to2' sets op1 to op2 index, etc)
  - type: ratio <list>
    description: set frequency ratio for operators
  - type: detune <list>
    description: set detuning frequency for operators
  - type: pan <list>
    description: set panning for operators
  - type: vol <list>
    description: set volume for operators
  - type: ratio'n' <float>
    description: set op ratio ('ratio1' sets op1, etc)
  - type: detune'n' <float>
    description: set op detuning ('detune1' sets op1, etc)
  - type: pan'n' <float>
    description: set op panning ('pan1' sets op1, etc)
  - type: vol'n' <float>
    description: set op volume ('pan1' sets op1, etc)

draft: false
---

[pm2~] is a 2 operators FM (actually phase modulation) synthesizer. Each oscillator can modulate itself and each other.