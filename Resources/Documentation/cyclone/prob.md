---
title: prob

description: probability matrix

categories:
 - object

pdcategory: cyclone, Random and Noise

arguments: (none)

inlets:
  1st:
  - type: bang
    description: chooses a weighted random number
  - type: list
    description: list of <value 1, value 2, weight>
  - type: float
    description: sets the current number

outlets:
  1st:
  - type: float
    description: random number based on prob function
  2nd:
  - type: bang
    description: when we reach a number for which there is no rule

methods: 
  - type: cleat
    description: clears the probability matrix
  - type: dump
    description: print out all rule
  - type: embed <float>
    description: <1> enables saving the matrix with the patch, <0> disables
  - type: reset <float>
    description: sets a value for when reaching a number with no rule

draft: true
---

[prob] outputs a weighted series of random numbers (1st order markov chain). It accepts lists of 3 numbers where the 3rd represents the weight/chance of going from value 1 to value 2, a bang will force [prob] to output. See below:
