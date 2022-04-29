---
title: qlist
description: text-based sequencer
categories:
- object
pdcategory: Misc
last_update: '0.35'
see_also:
- textfile
- text sequence
inlets:
  1st:
  - type: bang
    description: start sequence automatically.
  - type: tempo <float>
    description: set relative tempo.
  - type: rewind
    description: go to beginning (and stop).
  - type: next <float>
    description: single-step forward, optional float suppresses message sending.
  - type: print
    description: print contents to Pd window.
  - type: clear
    description: empty the qlist.
  - type: add <anything>
    description: add a message to a qlist.
  - type: add2 <anything>
    description: add a message to a qlist but don't terminate it.
  - type: read <symbol>
    description: read a file into qlist.
  - type: write <symbol>
    description: write contents to a file.
outlets:
  1st:
  - type: list
    description: list of leading numbers for the next message.
  2nd:
  - type: bang
    description: when reaching the end of sequence.
draft: false
---
The qlist object reads text files containing time-tagged Pd messages. You can have them sequenced automatically (by sending a "bang" message, possibly changing speed via "tempo" messages) or manually via the "rewind" and "next" messages.

To run the qlist automatically, send it a "read" message (the filename is relative to the directory the patch is in) and later a "bang." Messages in the file are separated by semicolons. Optional leading numbers are delay times in milliseconds. If the tempo is different from 1 the messages are sent faster or slower accordingly. Messages should start with a symbol giving the destination object. In the file "qlist.q" used here, the messages go to objects "this" and "that" which are receives below.

To run it manually, send "rewind" followed by "next". All messages not preceded by numbers are sent. As soon as a message starting with one or more numbers is encountered, the numbers are output as a list. There are many ways you could design a sequencer around this.

You can also record textual messages and save them to a file. Send "clear" to empty the qlist and "add" to add messages (terminated with semicolons.) The message, "add2" adds a list of atoms without finishing with a semicolon in case you want to make variable-length messages.
