---
title: textfile
description: read and write text files
categories:
- object
pdcategory: vanilla, Data Management, File Management
last_update: '0.33'
see_also:
- qlist
- text sequence
inlets:
  1st:
  - type: bang
    description: output a whole line and go to the next
outlets:
  1st:
  - type: anything
    description: lines stored in the textfile object
  2nd:
  - type: bang
    description: when reaching the end of sequence
methods:
  - type: rewind
    description: go to beginning (and stop)
  - type: clear
    description: empty the textfile
  - type: add <anything>
    description: add a message to textfile
  - type: add2 <anything>
    description: add a message but don't terminate it
  - type: set <anything>
    description: clear and add a message to textfile
  - type: print
    description: print contents to Pd window
  - type: read <symbol, cr>
    description: read a file (with optional 'cr' argument)
  - type: write <symbol, cr>
    description: write to a file (with optional 'cr' argument)
draft: false
---
'cr' = terminating lines only with carriage return (omitting semicolons.) You can read files this way too, in which case carriage returns are mapped to semicolons.

The textfile object reads and writes text files to and from memory. You can read a file and output sequential lines as lists, or collect lines and write them out. You can use this object to generate "models" for Gem, for instance.

To record textual messages and save them to a file, first send "clear" to empty the qlist and "add" to add messages (terminated with semicolons.) The message, "add2" adds a list of atoms without finishing with a semicolon in case you want to make variable-length messages.

You can also use this object simply for storing heterogeneous sequences of lists.
