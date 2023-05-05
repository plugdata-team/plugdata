---
title: numbox~

description: signal number box

categories:
 - object

pdcategory: ELSE, UI

arguments:

inlets:
  1st:
  - type: float
    description: sets generating value
  - type: signal
    description: value to display

outlets:
  1st:
  - type: signal
    description: input when monitoring or generated values instead

flags:
  - name: -size <float>
    description: font size (default: patch's, minimum 8)
  - name: -init <float>
    description: sets initial generating value
  - name: -width <float>
    description: sets digit width (default: 6, minimum 1)
  - name: -range <float, float>
    description: dragging range (default: 0, 0 - unlimited)
  - name: -bgcolor <f, f, f>
    description: sets background color in RGB (default 255 255 255)
  - name: -fgcolor <f, f, f>
    description: sets frontground color in RGB (default 0 0 0)
  - name: -ramp <float>
    description: sets ramp time in ms (default 10, minimum 0)
  - name: -rate <float>
    description: sets refresh rate in ms (default 100, minimum 15)

methods:
  - type: set <float>
    description: sets initialization and generating value with given float or sets current value if no float is given
  - type: size <float>
    description: sets font size
  - type: width <float>
    description: sets digits width
  - type: range <float, float>
    description: sets minimum and maximum dragging values
  - type: bgcolor <f, f, f>
    description: sets background color in RGB
  - type: fgcolor <f, f, f>
    description: sets foreground color in RGB
  - type: ramp <float>
    description: sets ramp time in ms
  - type: rate <float>
    description: sets refresh rate in ms

draft: false
---

[numbox~] is a signal GUI number box. It can be used to display/monitor signal values at a fixed rate or generate them with a ramp time.
