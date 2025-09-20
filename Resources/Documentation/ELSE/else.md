---
title: else

description: the ELSE binary

categories:
 - object

pdcategory: ELSE

inlets:

outlets:
  1st:
  - type: list
    description: ELSE version (major, minor, bugfix, status, status number)
  2nd:
  - type: list
    description: Pd version (major, minor, bugfix)
  3rd:
  - type: list
    description: Pd flavor information
  4th:
  - type: list
    description: ELSE binary directory
methods:
- type: about
  description: prints library information on the terminal
- type: version
  description: outputs version information as a list
- type: dir
  description: outputs ELSE binary directory

draft: true
---

The else binary does gives some basic information about the ELSE library in Pd's window at creation time (but only once if multiple else objects are loaded) and loads an object browser plugin. It accepts the "about" message that prints this basic information (version, release date, etc) on the terminal on demand and also accepts the "version" message that outputs the version information as a list.
