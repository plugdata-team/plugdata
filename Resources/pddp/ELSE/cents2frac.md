---
title: cents2frac

description: Cents/Fraction conversion

categories:
- object

pdcategory: Tuning / Scale Tools

arguments:
- description: conversion resolution
  type: float
  default: 1000, min 10

inlets:
  1st:
  - type: list
    description: cents value(s)
  - type: res <float>
    description: set conversion resolution

outlets:
  1st:
  - type: list
    description: converted fractional value(s)

draft: false
---

Use [cents2frac] to convert a list of cents to intervals defined as fraction symbols or lists.