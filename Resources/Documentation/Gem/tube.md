---
title: tube

description: renders a tube

categories:
 - object

pdcategory: ELSE, MIDI

arguments:
- type: float
  description: 1st circle diameter
  default: 1
- type: float
  description: 2nd circle diameter
  default: 1
- type: float
  description: height of the tube
  default: 1
- type: float
  description: number of segments
  default: 14

methods:
- type: draw <symbol>
  description: 'fill', 'line' or 'points'
- type: width <float>
  description: set line width
- type: numslices <float>
  description: set number of slices

inlets:
  1st:
  - type: gemlist
    description:
  2nd:
  - type: float
    description: 1st circle diameter
  3rd:
  - type: float
    description: 2nd circle diameter
  4th:
  - type: float
    description: height of the tube
  5th:
  - type: float
    description: X translation of 1st circle
  6th:
  - type: float
    description: Y translation of 1st circle
  7th:
  - type: float
    description: X rotation of 1st circle
  8th:
  - type: float
    description: Y rotation of 1st circle
  9th:
  - type: float
    description: X rotation of 2nd circle
  10th:
  - type: float
    description: Y rotation of 2nd circle

outlets:
  1st:
  - type: gemlist
    description: MIDI aftertouch

draft: false
---
