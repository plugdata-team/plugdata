---
title: openpanel
description: query for files or directories
categories:
- object
pdcategory: vanilla, File Management
last_update: '0.51'
see_also:
- savepanel
- pdcontrol
arguments:
- description: 'mode: 0 (file, default), 1 (directory), 2 (multiple files)'
  type: float
inlets:
  1st:
  - type: bang
    description: open dialog window to choose file(s) or directory
  - type: symbol
    description: set starting directory and open dialog window
outlets:
  1st:
  - type: symbol
    description: directory or file(s)' names
draft: false
---
When openpanel gets a "bang", a file browser appears on the screen. By default, if you select a file, its name appears on the outlet

A mode argument allow you to select a directory or multiple files.
