---
title: pdcontrol
description: communicate with pd and/or this patch
categories:
- object
pdcategory: Misc
last_update: '0.49'
inlets:
  1st:
  - type: browse <symbol>
    description: open a URL given by the symbol.
  - type: dir <float, symbol>
    description: output owning patch's directory. Optional float sets level (0, this
      patch, 1, its owner, and so on). Optional symbol sets a folder relative to the
      directory.
  - type: isvisible
    description: outputs float to specify if patch is visible (1) or not (0).
  - type: args <float>
    description: outputs patch's argument. Optional float sets level (0, this patch,
      1, its owner, and so on).
outlets:
  1st:
  - type: list
    description: list of args, dir symbol of visibility float.
draft: false
---
pdcontrol lets you open a URL in a web browser or communicate with the patch to get its owning directory, arguments or its visible/invisible state.

Optional argument to specify this patch (0), owning patch (1), its own owner (2), and so on, and optionally also a filename relative to the patch's directory. (Ownership number is silently reduced if owners don't exist, so here anything greater than zero is ignored.)
