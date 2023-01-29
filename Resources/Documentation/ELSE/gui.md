---
title: gui

description: run GUI commands

categories:
- object

pdcategory: GUI

arguments:

inlets:
  1st:
  - type: anything
    description: GUI comand

draft: false
---

[gui] sends a command directly to the tcl/tk interpreter (via 'sys_gui') used for Pd's gui.
