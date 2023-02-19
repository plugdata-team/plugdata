---
title: keycode
description: get key codes from keyboard

categories:
 - object

pdcategory: ELSE, UI

arguments:

inlets:

outlets:
  1st:
  - type: float
    description: 1 when key is pressed, 0 when released
  2nd:
  - type: float
    description: key code (layout independent)

draft: false
---
[keycode] outputs key codes from your keyboard based on their physical location, so it is layout independent (but please don't change layout after loading Pd and the object, this is problematic specially on Windows.. The right outlet outputs the code and the left output the key status (1 for pressed / 0 for released). The codes correspond to USB HID usage tables from <https://github.com/depp/keycode>.

