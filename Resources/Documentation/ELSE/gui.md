---
title: gui

description: Run GUI commands

categories:
- object

pdcategory: GUI

arguments: (none)

inlets:
  1st:
  - type: anything
    description: GUI comand

draft: false
---

[gui] sends a command directly to the tcl/tk interpreter (via 'sys_gui') used for Pd's gui.
