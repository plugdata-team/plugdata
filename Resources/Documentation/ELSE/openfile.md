---
title: openfile

description: open folders/files/weblinks

categories:
 - object
pdcategory: ELSE, Networking

arguments:
- type: symbol
  description: folder/file/weblink address
  default: current folder

inlets:
  1st:
  - type: bang
    description: open set file

outlets:

flags:
  - name: -h <anything>
    description: sets to hyperlink (GUI) mode, 1st element is file to open and name to display, further arguments overwrite display.

methods:
  - type: open <symbol>
    description: opens folder/file/weblink specified by the symbol, if no symbol is given, the current folder is opened

draft: false
---
[openfile] can be used to open folders and files in your computer and also weblinks.
