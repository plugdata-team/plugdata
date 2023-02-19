---
title: number~

description: signal generator/monitor

categories:
 - object

pdcategory: cyclone, UI, Signal Generators

arguments: (none)

inlets:
  1st:
  - type: signal
    description: the signal to be monitored/displayed
  - type: bang
    description:  alternate modes
  2nd:
    - type: float
      description: ramp time for signal generating

outlets:
  1st:
  - type: signal
    description:  the generated signal
  2nd:
    - type: float
        description: the monitored signal's value

flags:
  - name: @monitormode <float>
    description: non-0 sets monitor mode 
  - name: @sigoutmode <float>
    description: non-0 sets generator mode 
  - name: @ft1
    description: ramp time
  - name: @interval <float>
    description: display time interval in ms
  - name: @minimum <float>
    description: minimum generation value
  - name: @maximum <float>
    description: maximum generation value
  - name: @bgcolor <f, f, f>
    description: sets background color in RGB
  - name: @textcolor <f, f, f>
    description: text color in RGB

methods:
  - type: set <float>
    description: sets value but doesn't generate signal
  - type: list <f,f>
    description: sets value to generate and ramp time in ms
  - type: ft1 <float>
    description: sets ramp time in ms
  - type: interval <float>
    description: sets display time interval in ms  
  - type: bgcolor <f, f, f>
    description: sets background color in RGB
  - type: textcolor <f, f, f>
    description: sets text color in RGB
  - type: minimum <float>
    description: sets minimum generation value
  - type: maximum <float>
    description: sets maximum generation value

draft: false
---

Use [number~] to monitor or generate signals. This is an abstraction with limited functionality, not a proper clone.
The "tilde" symbol refers to monitor mode, and if you click on it, it changes to generator mode (marked with a down arrow, where you can click/drag and enter numbers). Clicking on the arrow switches it to monitor mode.
