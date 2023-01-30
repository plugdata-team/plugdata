---
title: tabgen

description: table generator

categories:
- object

pdcategory: ELSE, Arrays and Tables

arguments:
  - type: float
    description: (optional) sets array size at initialization
  - type: symbol
    description: set table name
    default: none
  - type: anything
    description: commands to generate functions
    default: none

flags:
  - name: -ms
    description: set table size to ms (default points)

inlets:
  1st:
  - type: anything
    description: commands to generate functions (details in [pd examples])

outlets:

methods:
  - type: size <float>
    description: resize table (needs to regenerate table)
  - type: set <symbol>
    description: sets table name
  - type: clear
    description: clear table

draft: false
---

[tabgen] is an abstraction that generates different functions for a given table name.

