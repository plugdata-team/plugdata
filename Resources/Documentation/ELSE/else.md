---
title: else

description: the ELSE binary

categories:
 - object

pdcategory: ELSE

arguments:

inlets:

outlets:
  1st:
  - type: list
    description: version (major, minor, bugfix, status, status number)

methods:
  - type: about
    description: prints library information on the terminal
  - type: version
    description: outputs version information as a list

draft: true #category?
---

The else binary does nothing but give some basic information about the ELSE library in Pd's window at creation time (but only once if multiple else objects are loaded). It accepts the "about" message that prints this basic information (version, release date, etc) on the terminal on demand and also accepts the "version" message that outputs the version information as a list.

