---
title: dir
description: access file directory

categories:
 - object

pdcategory: ELSE, File Management

arguments:
- type: float
  description: directory level, 0 — current patch's directory, 1 — parent's patch's, etc
  default: 0
- type: symbol
  description: directory to open
  default: relative directory level

inlets:
  1st:
  - type: bang
    description: outputs <n>, <directory> and dumps files

outlets:
  1st:
  - type: symbol
    description: the files from the directory
  2nd:
  - type: symbol
    description: the current directory
  3rd:
  - type: float
    description: <n> number of found files
  4th:
  - type: float
    description: 1 if opened a new directory, 0 if it couldn't open

flags:
  - name: -ext <symbol>
    description: sets file extension type


methods:
  - type: open <symbol>
    description: opens a directory and loads contents, no symbol opens binary's directory
  - type: reopen
    description: reloads contents of the last opened directory
  - type: reset
    description: resets to patch's directory
  - type: dir
    description: outputs the current directory in the right outlet
  - type: dump
    description: dump files from directory in the (second outlet)
  - type: n
    description: output number of found files in the (third outlet)
  - type: seek <float>
    description: seeks and output the file corresponding to that number
  - type: next
    description: increments seek value and outputs file
  - type: ext <symbol>
    description: sets extension

draft: false
---

[dir] accesses files from directories.

