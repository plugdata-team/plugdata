---
title: gui
description: Run gui commands

categories:
 - object

pdcategory: General

arguments:

inlets:
  1st:
  - type: anything
    description: gui command

outlets:

---

[gui] sends a command directly to the tcl/tk interpreter (via 'sys_gui') used for Pd's gui.

