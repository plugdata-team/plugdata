---
title: rand.hist

description: random values from histogram

categories:
 - object

pdcategory: ELSE, Random and Noise

arguments:
- type: list
  description: sets histogram
  default: 

inlets:
  1st:
  - type: bang
    description: generates a random output
  - type: list
    description: sets new histogram and clears memory

outlets:
  1st:
  - type: float
    description: random index output from histogram
 2nd:
  - type: bang
    description: bang when sequence is finished

flags:
  - name: -seed <float> 
    description: seed value
    default: unique internal
  - name: -size <float>
    description: sets histogram size
    default: 128
  - name: -eq <float>
    description: sets equal number of occurrences for all indexes
    default: 0
  - name: -u
    description: unrepeat mode
    default:

methods:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal
  - type: set <f, f> 
    description: set index and occurrence value
  - type: clear
    description: clears memory
  - type: inc <float>
    description: set index and increase occurrence by 1
  - type: dec <float>
    description: set index and decrease occurrence by 1
  - type: size <float>
    description: clears the memory and sets a new 'n' size
  - type: eq <float>
    description: sets an equal number of occurrences for all elements
  - type: unrepeat
    description: non-0 sets unrepeat mode

draft: false
---

[rand.hist] generates weighted random numbers based on a histogram (which specifies how many times an index is output). Below to the left we have a random sequence where 0 has a 30% chance of occurring, 1 has 20% and 2 has 50%. A list sets a new histogram. In "unrepeat" mode, a random sequence is output without repetition, below to the right, the random sequence contains index 0 three times and index 1 twice. When the sequence is finished, a bang comes out the right outlet. In this case, the 'restart' message clears the memory so you can start a new sequence.
