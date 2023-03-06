---
title: file glob
description: find pathnames matching a pattern
categories:
- object
pdcategory: vanilla, File Management
last_update: '0.52'
see_also:
- text
- array
- list
- file
- file define
- file handle
- file mkdir
- file which
- file stat
- file isfile
- file isdirectory
- file size
- file copy
- file move
- file delete
- file split
- file join
- file splitext
- file splitname
flags:
- name: -q
  description: set quiet verbosity
- name: -v
  description: set loud verbosity
inlets:
  1st:
  - type: symbol
    description: pattern to be found
outlets:
  1st:
  - type: list
    description: found files/directories as a path symbol and type (file <0>, directory <1>)
  2nd:
  - type: bang
    description: if nothing is found or an error occurs
methods:
  - type: verbose <float>
    description: set verbosity on or off
draft: false
---
cross-platform notes on globbing:

[file glob] attempts to unify the behaviour of wildcard matching on different platforms. as such, it does not support all features of a given pattern matching implementation (or only accidentally).

the following rules should help you to write patches that use platform independent globbing.

- the pattern may contain the wildcards '*' (for any number of characters) and '?' (for a single character) in the last path component. no other wildcards are supported.

- the behaviour of patterns that contain wildcards in a path component other than the last one is *undefined* (and platform dependent). DO NOT USE THIS.

- patterns ending with '/' will ONLY match directories

- patterns ending with anything else will match files AND directories

- files/dirs starting with a "." only match if the matching pattern explicitly contains the leading dot.

- the special files/dirs "." and ".." only match if requested explicitly, never with a wildcard pattern.
