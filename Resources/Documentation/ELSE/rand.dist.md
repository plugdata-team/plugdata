---
title: rand.dist

description: random distribution 

categories:
 - object

pdcategory: ELSE, Random and Noise

arguments:
- type: symbol
  description: sets table name
  default: none
- type: float
  description: sets minimum output 
  default: 0
- type: float
    description: sets maximum output
    default: 127

inlets:
  1st:
  - type: bang
    description: generates a random output
  - type: float
    description: range from 0-127) get quantile
  2nd:
  - type: float
    description: lowest random value
  3rd:
  - type: float
    description: highest random value

outlets:
  1st:
  - type: float
    description: random values

flags:
  - name: -seed <float> 
    description: seed value

methods:
  - type: seed <float>
    description: a float sets seed, no float sets a unique internal
  - type: set <symbol> 
    description: sets array name

draft: false
---

[rand.dist] is an abstraction based on [array quantile] and generates random numbers that follow a probability density function (given by an array). A float input (like from an external random source) outputs the array quantile instead. The first argument is the table name containing the probability density function. The output range is from 0 to 127 by default, but you can change that with the 2nd and 3rd argument or the 2nd or 3rd inlet. The output resolution is the range divided by the table maximum index (127 in the example below, so the 0-127 range outputs integers).
