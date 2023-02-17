---
title: standard~

description: standard map chaotic generator

categories:
 - object
 
pdcategory: ELSE, Random and Noise, Signal Generators

arguments:
- type: float
  description: sets frequency in Hz
  default: Nyquist
- type: float
  description: sets k
  default: 1
- type: float
  description: sets initial value of x[n-1]
  default: 0.5
- type: float
  description: sets initial value of y[n-1] 
  default: 0
  
inlets:
  1st:
  - type: float/signal
    description: frequency in Hz (negative values accepted)
  - type: list
    description: 2 floats set x[n-1] and y[n-1], respectively

outlets:
  1st:
  - type: signal
    description: standard map chaotic signal

methods:
  - type: k <float>
    description: sets the value of k

info: This object was based on SuperCollider's "StandardN" UGEN;
The standard map is an area preserving map of a cylinder discovered by the plasma physicist Boris Chirikov.

draft: false
---

[standard~] is a standard map chaotic generator. The sound is generated with the difference equations;
y[n] = (y[n-1] + k * sin(x[n-1])) % 2pi;
x[n] = (x[n-1] + y[n]) % 2pi;
out = (x[n] - pi) / pi;
The output rate of the equation is given in Hz (default: Nyquist).
