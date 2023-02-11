---
title: until
description: looping mechanism
categories:
- object
pdcategory: vanilla, Data Math, Logic
last_update: '0.28'
inlets:
  1st:
  - type: bang
    description: stops the loop
  - type: float
    description: set number of iterations in the loop
  2nd:
  - type: bang
    description: stops the loop
outlets:
  1st:
  - type: bang
    description: bangs in a loop
draft: false
---
The until object's left inlet starts a loop in which it outputs "bang" until its right inlet gets a bang which stops it. If you start "until" with a number,  it iterates at most that number of times,  as in the Max "uzi" object.
