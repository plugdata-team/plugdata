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
- description: optional float sets a constant value to all indexes 
  default: 0
  method: const <float>
- description: resizes the array.
  method: resize <float>
- description: first element sets array size, remaining elements set amplitude of
    sine components.
  method: sinesum <list>
- description: first element sets array size, remaining elements set amplitude of
    cosine components.
  method: cosinesum <list>
- description: prints array information (name, type, size