---
title: scope~

description: oscilloscope display

categories:
 - object

pdcategory: cyclone, UI

arguments: (none)

inlets:
  1st:
  - type: signal
    description: signal to be displayed in the X axis
  - type: float
    description: sets samples per line
  2nd:
  - type: signal
    description: signal to be displayed in the Y axis
  - type: float
    description: sets the number of lines in buffer

outlets:

flags:
  - name: @fgcolor <f,f,f>
    description: sets front/signal RGB color
  - name: @bgcolor <f,f,f>
    description: sets background RGB color
  - name: @gridcolor <f,f,f>
    description: sets grid RGB color
  - name: @range <f,f>
    description: sets vertical range 
  - name: @trigger <float>
    description: 0 (none), 1 (up) or 2 (down) 
  - name: @triglevel <float>
    description: sets threshold level for the trigger mode 
  - name: @bufsize <float>
    description: sets the number of lines in buffer (8-256)
  - name: @calccount <float>
    description: sets samples per line
  - name: @delay <float>
    description: onset time delay between displays
  - name: @drawstyle <float>
    description: <1> sets alternate drawing style 
  - name: @dim <f,f>
    description: sets width/height
  - name: @receive <symbol>
    description: receive symbol  

methods:
  - type: dim <f,f>
    description: sets width/height 
    default: 130 130
  - type: calccount <float>
    description: sets samples per line
    default: 
  - type: bufsize <float>
    description: sets the number of lines in buffer (8-256)
    default: 128
  - type: range <f,f>
    description: sets vertical range 
    default: -1 1
  - type: fgcolor <f,f,f>
    description: sets front/signal RGB color
    default: 
  - type: bgcolor <f,f,f>
    description: sets background RGB color
    default:
  - type: gridcolor <f,f,f>
    description: sets grid RGB color
    default:
  - type: drawstyle <float>
    description: <1> sets alternate drawing style 
    default: 0
  - type: trigger <float>
    description: 0 (none), 1 (up) or 2 (down) 
    default: 0
  - type: triglevel <float>
    description: sets threshold level for the trigger mode 
    default: 0
  - type: delay <float>
    description: onset time delay between displays
    default: 0
  - type: receive <symbol>
    description: receive symbol 
    default: empty

draft: true
---

[scope~] displays a signal in the style of an oscilloscope.
