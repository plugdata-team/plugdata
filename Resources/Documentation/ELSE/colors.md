---
title: colors

description: pick/convert colors

categories:
 - object

pdcategory: ELSE, UI

arguments:
inlets:
  1st:
  - type: bang
    description: output/convert color

outlets:
  1st:
  - type: anything
    description: color values (in RGB, hex, iemgui or ds)

flags:
  - name: -rgb
  description: sets to RGB list output (default hexadecimal)
  - name: -iemgui
  description: sets to iemguis output (default hexadecimal)
  - name: -ds
  description: sets to Data Structures output (default hexadecimal)

methods:
  - type: pick
    description: open color picker
  - type: rgb <f, f, f>
    description: RGB (Red, Green, Blue) color values
  - type: hex <symbol>
    description: hexadecimal color value
  - type: iemgui <float>
    description: iemgui's color format value
  - type: ds <float>
    description: Data Structures' color value
  - type: cmyk <f, f, f, f>
    description: CMYK (Cyan, Magenta, Yellow, and Key) color values
  - type: hsl <f, f, f>
    description: HSL (Hue, Saturation, and Lightness) color values
  - type: hsv <f, f, f>
    description: HSV (Hue, Saturation, and Value) color values
  - type: gray <float>
    description: grayscale value from 0 to 100
  - type: to <symbol>
    description: set conversion to "rgb," "hex," "gui" or "ds"

draft: false
---

[colors] is a color picker and converter. You can set the output format and also convert from other input formats. Colors that you pick and colors you send to the object are stored and recalled next time you open the color picker. This object is useful for setting color properties inside abstractions.
