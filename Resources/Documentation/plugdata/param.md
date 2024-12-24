---
title: param

description: interact with DAW automation parameters

categories:
- object

pdcategories: PlugData, UI

methods:
  - type: create
    description: activate the automation paramter
  - type: range <float, float>
    description: set parameter range
  - type: mode <float>
    description: set parameter mode (1=float, 2=integer, 3=logarithmic, 4=exponential)

arguments:
  - type: symbol
    description: parameter name
  - type: float
    description: 1 enables automatically handling change state when interacting with GUI objects
    default: 0

inlets:
  1st:
  - type: float
    description: set DAW parameter value
  2nd:
  - type: float
    description: inform DAW about change state (0 = reading, 1 = writing)

outlets:
  1st:
  - type: float
    description: received DAW parameter value

draft: false
---
[param] sends and receives automation parameters to the DAW. You'll have to create an automation parameter in the sidebar.
