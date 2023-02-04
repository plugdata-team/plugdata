---
title: gui

description: run GUI commands

categories:
- object

pdcategory: ELSE, UI

arguments:

inlets:
  1st:
  - type: anything
    description: GUI command

draft: false
---

[gui] sends a command directly to the tcl/tk interpreter (via 'sys_gui') used for Pd's GUI.
