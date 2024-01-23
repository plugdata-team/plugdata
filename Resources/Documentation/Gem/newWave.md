---
title: newWaves
description: renders a waving square (mass-spring-system)
categories:
  - object
pdcategory: Gem, Graphics
arguments:
  - type: float
    description: X grid resolution
    default: 3
  - type: float
    description: Y grid resolution
    default: X value
methods:
   - type: K1
    description: weight factor
    default: 0.05
  - type: D1
    description: damping factor
    default: 0.1
  - type: K2
    description: weight factor
    default: 0
  - type: D2
    description: damping factor
    default: 0
  - type: K3
    description: weight factor
    default: 0
  - type: D3
    description: damping factor
    default: 0
  - type: position <float> <float> <float>
    description: clamp the node at (X Y) to a certain height and release it
  - type: noise <float>
    description: add a random force ( -val < force < +val) to all nodes
  - type: force <float> <float> <float>
    description: apply a force of value of arg3 onto the wave at position (arg1, arg2)

inlets:
  1st:
    - type: gemlist
      description:
  2nd:
    - type: list
      description: size (dimX & dimY)
  3rd:
    - type: float
      description: height
  4th:
    - type: float
      description: action
outlets:
  1st:
    - type: gemlist

draft: false
---
