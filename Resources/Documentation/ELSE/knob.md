---
title: knob

description: knob GUI

categories:
 - object

pdcategory: ELSE, UI

arguments:
inlets:
  1st:
  - type: float
    description: sets and outputs value
  - type: bang
    description: outputs value

outlets:
  1st:
  - type: float
    description: knob's value

flags:
  - name: -set <float>
    description: sets value (no output)
  - name: -init <float>
    description: sets initialization value, no float sets current 
    default: 0
  - name: -size <float>
    description: - sets size in pixels 
    default: 50
  - name: -arc <float>
    description: - non zero sets to arc drawing mode 
    default: 1
  - name: -outline <float>
    description: - non zero sets to outline drawing mode 
    default: 0
  - name: -bgcolor <list>
    description: sets background color in hexdecimal or RGB list 
    default: #dfdfdf
  - name: -fgcolor <list>
    description: sets foreground color in hexdecimal or RGB list 
    default: #000000
  - name: -circular <float>
    description: non zero sets to circular dragging mode 
    default: 0
  - name: -angle <float>
    description: sets angular range value in degrees 
    default: 320
  - name: -offset <float>
    description: sets angular offset in degrees 
    default: 0
  - name: -send <symbol>
    description: sets send symbol 
    default: empty
  - name: -receive <symbol>
    description: sets receive symbol 
    default: empty
  - name: -exp <float>
    description: sets exponential value 
    default: 1
  - name: -ticks <float>
    description: sets number of ticks  
    default: 0
  - name: -discrete <float>
    description: non zero sets to discrete mode
    default: 0  
  - name: -jump <float>
    description: non zero sets to jump on click mode
    default: 0

methods:
  - type: set <float>
    description: sets value (no output)
  - type: init <float>
    description: sets initialization value, no float sets current 
    default: 0
  - type: size <float>
    description: - sets size in pixels 
    default: 50
  - type: arc <float>
    description: - non zero sets to arc drawing mode 
    default: 1
  - type: outline <float>
    description: - non zero sets to outline drawing mode 
    default: 0
  - type: bgcolor <list>
    description: sets background color in hexdecimal or RGB list 
    default: #dfdfdf
  - type: fgcolor <list>
    description: sets foreground color in hexdecimal or RGB list 
    default: #000000
  - type: circular <float>
    description: non zero sets to circular dragging mode 
    default: 0
  - type: angle <float>
    description: sets angular range value in degrees 
    default: 320
  - type: offset <float>
    description: sets angular offset in degrees 
    default: 0
  - type: send <symbol>
    description: sets send symbol 
    default: empty
  - type: receive <symbol>
    description: sets receive symbol 
    default: empty
  - type: exp <float>
    description: sets exponential value 
    default: 1
  - type: ticks <float>
    description: sets number of ticks  
    default: 0
  - type: discrete <float>
    description: non zero sets to discrete mode
    default: 0  
  - type: jump <float>
    description: non zero sets to jump on click mode
    default: 0

draft: false
---


