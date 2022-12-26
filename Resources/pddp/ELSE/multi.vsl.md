---
title: multi.vsl

description:

categories:
- object

pdcategory:

arguments:
- description:
  type:
  default:

inlets:
  1st:
  - type:
    description:
  2nd:
  - type:
    description:

outlets:
  1st:
  - type:
    description:


flags: 
 - name: -n <float>
   description: sets number of sliders size (default: 8), f 72;
 - name: -range <float \, float>
   description: sets slidrs' range (default: 0 \, 127), f 72;
 - name:  -name <symbol>
   description: sets arrays name (default: internal), f 72;
 - name: -jump <float>
   description: non zero sets jump on click mode (default: 0), f 72;
 - name: -dim <float \, float>
   description: sets x/y dimensions (default: 200 127), f 72;
 - name: -init <float>
    description: non zero sets to init mode (default: 0), f 72;
 - name: -send <symbol>
    description: sets send symbol (default: empty), f 72;
 - name: -receive <symbol>
    description: sets receive symbol (default: empty), f 72;
 - name: -bgcolor <f \, f \, f>
    description: sets background color in RGB (default: 255 255 255), f 72;
 - name: -fgcolor <f \, f \, f>
    description: sets frontground color in RGB (default: 220 220 220), f 72;
 - name: -linecolor <f \, f \, f>
    dscription: sets line color in RGB (default: 0 0 0), f 72;
 - name: -set <list>
    description: sets slider's values (default: 0 0 0 0 0 0 0 0), f 72;
 - name -mode <float>
    description: non zro sets to 'list mode' (default 0), f 72;




draft: false
---

[multi.vsler] is a multi slider GUI abstraction.
