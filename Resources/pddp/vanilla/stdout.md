---
title: stdout
description: write messages to standard output
categories:
- object
see_also:
- pd~
pdcategory: "'EXTRA' (patches and externs in pd/extra)"
last_update: '0.48'
inlets:
  1st:
  - type: anything
    description: any message to be written/sent to standard output.
flags:
- flag: -cr
  description:  omit trailing semicolon in output (like [print]).
- flag: -b
  description:  sets to binary mode.
- flag: -binary
  description:  sets to binary mode.
- flag: -f
  description: same as -flush.
- flag: -flush
  description: flush the output after each message (default on W32).
- flag: -nf
  description: same as -noflush.
- flag: -noflush
  description: do not flush the output after each message.  
draft: false
---
The 'stdout' object is useful in conjunction with the pd~ object, which starts a Pd sub-process. Messages sent to the sub-process standard output appear on the left output of the pd~ object in the owning process. This might also be useful in other situations. Note that there's no corresponding "stdin" object - there seems to be no one canonical way such a thing should act.