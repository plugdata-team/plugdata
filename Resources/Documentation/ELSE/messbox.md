---
title: messbox

description: GUI message box

categories:
 - object

pdcategory: ELSE, UI

arguments:

inlets:
  1st:
  - type: bang
    description: outputs the message
  - type: anything
    description: sets messages with "$1" "$2", etc and outputs them

outlets: 
  1st:
  - type: anything
    description: messages

flags:
- name: -fontsize <float>
   description: font size
- name: -size <f, f>
  description: size
- name: -fgcolor <f, f, f>
  description: foreground color
- name: -bgcolor <f, f, f>
  description: background color
- name: -bold
  description: non-0 sets to bold

methods:
  - type: set <anything>
    description: sets message
  - type: append
    description: appends to the message
  - type: dim <f, f>
    description: sets width and height
  - type: fontsize <float>
    description: sets font size
  - type: bold
    description: non-0 sets to bold
  - type: -bgcolor <f, f, f>
    description: sets background color
  - type: fgcolor <f, f, f>
    description: sets text color

draft: false
---

The [messbox] object is an object you can type in messages while in run mode. It's like atom boxes but does all that a message box does. Any message type is possible. It deals with dollar signs, comma and semicolons in the same way as regular message boxes. See [pd syntax] for more details.
