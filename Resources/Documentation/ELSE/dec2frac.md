---
title: dec2frac

description: decimal/Fraction conversion

categories:
- object

pdcategory: ELSE, Data Math, Converters

arguments:
- description: conversion resolution (min 10)
  type: float
  default: 1000

inlets:
  1st:
  - type: list
    description: decimal value(s)

outlets:
  1st:
  - type: list
    description: converted fractional value(s)

flags:
  - name: -list
    description: sets to list mode (output fraction as a list)

methods:
  - type: res <float>
    description: set conversion resolution

draft: false
---

Use [dec2frac] to convert a list of decimals to intervals defined as fraction symbols or lists.

