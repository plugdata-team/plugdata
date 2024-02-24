---
title: notedur2ratio

description: convert note duration to fraction or float

categories:
- object

pdcategory: ELSE, Converters, Triggers and Clocks

arguments:

inlets:
  1st:
  - type: symbol
    description: in note duration syntax to convert

outlets:
  1st:
  - type: float/symbol
    description: converted duration to fraction or float

flags:
- name: -f
  description: sets output to float
  default: fraction

draft: false
---

[notedur2ratio] converts from a note duration syntax to a fractional notation or a float. The syntax is: '1n' for whole note, '2n' for half note, '4n' for quarter note, '8n' for eighth note (and so on down to 128n). Dotted notes are possible and you can add tuplets to the symbol in the [x:y] format or you can add the following letters to specify most common tuplets: 'd' for duplet, 't' for triplet, 'q' for quintuplet, 's' for septuplet and 'n' for nonuplet. Tied notes are also possible with "_". Details in the examples below.
