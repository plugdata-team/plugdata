---
title: mousestate
description: retrieve mouse data
categories:
 - object
pdcategory: cyclone, UI
arguments:
inlets:
  1st:
  - type: bang
    description: polls position data
outlets:
  1st:
  - type: float
    description: click status: 1 <button is pressed>, 0 <button is released>
  2nd:
  - type: float
    description: cursor 'x' (horizontal) position (in pixels)
  3rd:
  - type: float
    description: cursor 'y' (vertical) position (in pixels)
  4th:
  - type: float
    description: delta 'x' (position difference in pixels) from last poll
  5th:
  - type: float
    description: delta 'y' (position difference in pixels) from last poll

methods:
  - type: poll
    description: poll position data automatically when mouse cursor moves
  - type: nopoll
    description: does not poll position data (only via bang)
  - type: mode <float>
    description: sets mode, which defines the coordinate (0, 0) point in relation to (0: screen, 1: patch window, 2: front most window)
  - type: zero
    description: sets current position to (0, 0) of Pd's coordinate system
  - type: reset
    description: resets (0, 0) coordinates to the default

---

[mousestate] reports click status, cursor position coordinates and cursor delta. The click report is always given (and sampled every 50 ms), the rest needs to be polled.

