---
title: presets

description: presets manager

categories:
- object

pdcategory: ELSE, Data Management

arguments:
- description: one or more receive names
  type: list
  default: none

inlets:
  1st:
  - type: float
    description: sets a preset number and loads it
  - type: bang
    description: save preset (same as 'save' without a float)
  - type: list
    description: import presets from a list
  2nd:
  - type: float
    description: interpolation value (when in interpolation mode)

outlets:
  1st:
  - type: list
    description: presets as a list when receiving "export" message
  2nd:
  - type: bang
    description: when the preset is empty

flags:
  - name: -n <float>
    description: sets number of presets (default: 64)
  - name: -file <symbol>
    description: set a file to import from (default: none)
  - name: -morphtime <float>
    description: sets morph time (default: 0)
  - name: -morph <f, f>
    description: sets morph time and grain (default: 0, 20)
  - name: -interp
    description: sets to interpolation mode (default: no interpolation)
  - name: -exp <float>
    description: sets exponential factor for morph/interpolation (default: 1)

methods:
  - type: set <float>
    description: sets a preset number
  - type: load <float>
    description: load from a preset number. If no float is given, the last set preset number is loaded
  - type: save <float>
    description: save to a preset number. If no float is given, the last set preset is saved
  - type: n <float>
    description: sets number of presets
  - type: morph <f, f>
    description: sets morph time and grain for morphing between presets
  - type: interp <f>
    description: sets to interpolation mode
  - type: exp <f>
    description: sets exponential factor for interpolation/morphing
  - type: clear <float>
    description: clear data from preset number, no float clears all presets
  - type: show
    description: open the preset window so you can see/edit values
  - type: hide
    description: close preset window
  - type: args <list>
    description: replaces receive names arguments
  - type: read <symbol>
    description: import presets from a file, no symbol opens dialog window
  - type: write <symbol>
    description: write presets to file, no symbol opens dialog window
  - type: export
    description: export presets as a list


draft: false
---

[presets] manages presets for patches and abstractions and has interpolation and morphing features. Change the number and symbol below, save a preset and the patch, reopen to recall the presets. Use the "read" message to restore to this example's default.

