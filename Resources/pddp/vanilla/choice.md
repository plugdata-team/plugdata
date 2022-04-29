---
title: choice
description: search for a best match to an incoming list
categories:
- object
see_also: {}
pdcategory: "'EXTRA' (patches and externs in pd/extra)"
last_update: '0.30'
inlets:
  1st:
  - type: print
    description: post debugging information in the print out window.
  - type: add <list>
    description:  add vectors into the objects.
  - type: clear
    description: delete all stored vectors.
outlets:
  1st:
  - type: float
    description: index of best match (from zero).
argumets:
- type: float
  description: non zero avoids repeated output (default 0).
draft: false
---
The choice object holds a list of vectors, each having up to ten elements. When sent a list of numbers, it outputs the index of the known vector that matches most closely. The quality of the match is the dot product of the two vectors after normalizing them, i.e., the vector whose direction is closest to that of the input wins.

If given a nonzero creation argument, choice tries to avoid repetitious outputs by weighting less recently output vectors preferentially.

You can use this to choose interactively between a number of behaviors depending on their attributes. For example, you might have stored a number of melodies, of which some are syncopated, some chromatic, some are more than 100 years old, some are bugle calls, and some are Christmas carols. You could then ask to find a syncopated bugle call (1, 0, 0, 1, 0) and you'll get the thing most closely matching the request.

You can use numbers other than 0 and 1 to indicate relative strengths of the attributes, or even use negative numbers to indicate opposites, either in the incoming lists or in the stored ones.