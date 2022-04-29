---
title: garray
description: graphical array (messages received via array name).
categories:
- object
pdcategory: Arrays & Tables
last_update: '0.52'
see_also:
- inlet
- namecanvas
- array
methods:
- description: sets values into array, fisty element is starting index (from 0).
  method: list
- description: optional float sets a constant value to all indexes (default 0).
  method: const <float>
- description: resizes the array.
  method: resize <float>
- description: first element sets array size, remaining elements set amplitude of
    sine components.
  method: sinesum <list>
- description: first element sets array size, remaining elements set amplitude of
    cosine components.
  method: cosinesum <list>
- description: prints array information (name, type, size) on the terminal window.
  method: print
- description: normalizes to maximum absolute amplitude set by optional float (default
    1).
  method: normalize <float>
- description: load contents from a text file with the symbol name.
  method: read <symbol>
- description: save contents to a text file with the symbol name.
  method: write <symbol>
- description: renames the array to a new name defined by the given symbol.
  method: rename <symbol>
- description: sets rectangle bounds (xmin, ymax, xmax, ymin).
  method: bounds <list>
- description: sets a point to put a tick, the interval between ticks, and the number
    of ticks overall per large tick.
  method: xticks <list>
- description: same as above for the 'y' axis.
  method: yticks <list>
- description: first element is a label offset, remaining are the values to label.
  method: xlabels <list>
- description: first element is a label offset, remaining are the values to label.
  method: ylabels <list>
- description: zero hides array, non zero shows it.
  method: vis <float>
- description: sets array's width (default 1).
  method: width <float>
- description: sets array color in the same format as Data Structures.
  method: color <float>
- description: sets display style (0-point, 1-polygon, 2-bezier).
  method: style <float>
- description: non zero allows editing with the mouse, zero prevents it.
  method: edit <float>
draft: false
---
Input is via send names
