---
title: cyclone_comment
description: comment GUI
categories:
 - object
pdcategory: cyclone, UI
arguments:
inlets:
  1st:
  - type: set <anything>
    description: sets comment
outlets:

flags:
  - name: @fontname <symbol>
    description: default: 'dejavu sans mono' or 'menlo' for mac
  - name: @fontface <float>
    description: default 0
  - name: @fontsize <float>
    description: default patch's
  - name: @textjustification <float>
    description: default 0
  - name: @textcolor <f, f, f>
    description: default 0 0 0
  - name: @bgcolor <f, f, f>
    description: default 255 255 255
  - name: @bg <float>
    description: default 0
  - name: @text <anything>
    description: sets comment (default "comment")
  - name: @underline <float>
    description: default 0
  - name: @receive <symbol>
    description: default 'empty'

methods:
  - type: set <anything>
    description: sets comment
  - type: append <anything>
    description: appends a message to the comment
  - type: prepend <anything>
    description: prepends a message to the comment
  - type: fontsize <float>
    description: sets font size
  - type: fontname <symbol>
    description: sets font type
  - type: fontface <float>
    description: sets face: 0-normal, 1-bold, 2-italic, 3-bold+italic
  - type: underline <float>
    description: non-0 sets underline
  - type: textjustification <f>
    description: sets justification (0: left, 1: center, 2: right)
  - type: textcolor <f, f, f>
    description: sets RGB text color (values from 0 to 255)
  - type: bgcolor <f, f, f>
    description: sets RGB background color (values from 0 to 255)
  - type: bg <float>
    description: background flag (0 suppresses background)
  - type: receive <symbol>
    description: sets receive symbol

---

[comment] is a GUI meant to be only a comment (a label or a note) that can be typed directly into it when in Edit mode. It is widely used in cyclone's documentation and (unlike pd vanilla's comments) allows you to set font, size, color, background color, bold, italic, underline and justification. This object is not being fully compliant to Max6+ versions!

