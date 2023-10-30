---
title: keypress

description: key learn

categories:
 - object

pdcategory: ELSE, UI

arguments:
- type: symbol
  description: sets a key name
  default: none

outlets:
  1st:
  - description: from learned key depending on the mode
    type: bang/float
  2nd:
  - description: learned key
    type: symbol

flags:
  - name: -toggle
    description: sets to toggle output

methods:
  - type: learn
    description: activate key learn
  - type: toggle <float>
    description: nonzero sets to toggle mode
  - type: query
    description: send stored input on right outlet
  - type: teach <symbol>
    description: teach a specific key
  - type: forget
    description: forget input

draft: false
---

[keypress] uses key presses of a single key to send a bang or a toggle. You can set the key symbol as an argument or ask the object to "learn" a pressed key (useful for kys like "shift" and stuff), which is then saved as an argument in the owning patch. Note that autorepeated keys are filtered! Key presses in edit mode are also filtered!