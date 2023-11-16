---
title: circuit~

description: analog circuitry simulator

categories:
 - object

pdcategory: ELSE, Signal Generators

arguments:
- type: list
  description: text description of the circuit
  default: 0

inlets:
  1st:
  - description: input signal for $s0
    type: signal
  - description: reset and start simulation
    type: bang
  nth:
  - description: input signal for $s$nth
    type: signal

outlets:
  nth:
  - description: output signal from probe $nth
    type: signal

flags:
  - name: -dcblock <float>
    description: enable/disable dc offset blocking
  - name: -oversample <float>
    description: set amount of oversampling
  - name: -iter <float>
    description: set transistors/diodes quality

methods:
  - type: enable <float>
    description: sets a new text note
  - type: dcblock <float>
    description: appends a message to the note
  - type: oversample <float>
    description: appends a message to the note
  - type: iter <float>
    description: appends a message to the note
  - type: reset
    description: reset simulation

draft: false
---

[circuit~] simulates analog circuitry from a text description of the circuit separated by semicolons. Voltage, current, resistance and potmeter can be modulated by an input signal by passing '$sn' as the first (where 'n' is the inlet number indexed from 1). To create an audio input, use a voltage source with a modulatable parameter. An audio outlet will be created for every probe component. The object can be quite CPU intensive!
