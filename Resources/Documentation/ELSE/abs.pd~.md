---
title: abd.pd~

description: simple wrapper for pd~ subprocess

categories:
 - object

pdcategory: ELSE, Audio I/O, Data Management

arguments:
  - type: symbol
    description: '.pd' file to load
    default: none

methods:
  - type: start <symbol>
    description: start a subprocess, no symbol restarts last one
  - type: stop
    description: stop the subprocess
  - type: show
    description: open pd window

inlets:
  1st:
  - type: signal
    description: the left input to pd~ subprocess
  - type: anything
    description: messages to the subprocess
  2nd:
  - type: signal
    description: the right input to pd~ subprocess

outlets:
  1st:
  - type:
    description: sig

draft: false
---

[abs.pd~] loads a pd file into a subprocess via [pd~] and makes things more convenient like making the subprocess able to receive MIDI data from MIDI devices connected to the parent process (thanks to [sendmidi]). Also, you can open the subprocess patch by clicking on the object.
