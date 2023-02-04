---
title: teeth~

description: teeth comb filter

categories:
 - object

pdcategory: cyclone, Filters, Effects

arguments:
- type: float
  description: maximum delay time in ms 
  default: 10
- type: float
  description: feedforward delay time in ms
  default: 0
- type: float
  description: feedback delay time in ms
  default: 0
- type: float
  description: input gain coefficient
  default: 0
- type: float
  description: feedforward gain coefficient
  default: 0
- type: float
  description: feedback gain coefficient
  default: 0

inlets:
  1st:
  - type: signal
    description: signal to pass through teeth filter
  - type: list
    description:  updates all 6 arguments
  2nd:
  - type: signal/float
    description: feedforward delay time in ms
  3rd:
  - type: signal/float
    description: feedback delay time in ms 
  4th:
  - type: signal/float
    description: input gain coefficient
  5th:
  - type: signal/float
    description: feedforward gain coefficient
  6th:
  - type: signal/float 
    description: feedback gain coefficient

outlets:
  1st:
  - type: signal
    description:

methods:
 - type: clear
   description: clears buffer
   default:

draft: true
---

[teeth~] is a comb filter with independent time control of feedforward and feedback delays.
