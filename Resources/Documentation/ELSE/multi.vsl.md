---
title: multi.vsl

description: multi vertical slider

categories:
- object

pdcategory: ELSE, UI

arguments:

inlets:
  1st:
  - type: bang
    description: outputs all slider's values (depends on mode)
  - type: list
    description: sets and dumps sliders values from the first (0)

outlets:
  1st:
  - type: list
    description: slider number / slider value in the default mode or all values as a list in the "list mode"
  2nd:
  - type: list
    description: list of values when receiving export message

flags: 
 - name: -n <float>
   description: sets number of sliders size (default: 8), f 72;
 - name: -range <float \, float>
   description: sets slider's' range (default: 0 \, 127), f 72;
 - name:  -name <symbol>
   description: sets arrays name (default: internal), f 72;
 - name: -jump <float>
   description: non-0 sets jump on click mode (default: 0), f 72;
 - name: -dim <float \, float>
   description: sets x/y dimensions (default: 200 127), f 72;
 - name: -init <float>
    description: non-0 sets to init mode (default: 0), f 72;
 - name: -send <symbol>
    description: sets send symbol (default: empty), f 72;
 - name: -receive <symbol>
    description: sets receive symbol (default: empty), f 72;
 - name: -bgcolor <f \, f \, f>
    description: sets background color in RGB (default: 255 255 255), f 72;
 - name: -fgcolor <f \, f \, f>
    description: sets foreground color in RGB (default: 220 220 220), f 72;
 - name: -linecolor <f \, f \, f>
    description: sets line color in RGB (default: 0 0 0), f 72;
 - name: -set <list>
    description: sets slider's values (default: 0 0 0 0 0 0 0 0), f 72;
 - name -mode <float>
    description: non zro sets to 'list mode' (default 0), f 72;


methods:
  - type: mode <float>
    description: non-0 sets to 'list mode'
  - type: dump
    description: outputs values sequentially as slider number / value
  - type: set <list>
    description: sets values from the index defined in the first float
  - type: setall <float>
    description: sets all sliders' values
  - type: get <list>
    description: gets sliders values from the first float
  - type: dim <f, f>
    description: sets horizontal/vertical dimensions
  - type: width <float>
    description: sets horizontal size
  - type: height <float>
    description: sets vertical size
  - type: range <f, f>
    description: sets sliders' output range
  - type: n <float>
    description: sets number of sliders
  - type: jump <float>
    description: non-0 sets jump on click mode
  - type: import <list>
    description: sets number of sliders and values (and dumps them)
  - type: export
    description: outputs sliders values as a list
  - type: init <float>
    description: non-0 sets to jump on click mode
  - type: send <symbol>
    description: sets send name
  - type: receive <symbol>
    description: sets receive name
  - type: bgcolor <f, f, f>
    description: sets background color in RGB
  - type: fgcolor <f, f, f>
    description: sets foreground color in RGB
  - type: linecolor <f, f, f>
    description: sets line color in RGB

draft: false
---

[multi.vsl] is a multi slider GUI abstraction.
