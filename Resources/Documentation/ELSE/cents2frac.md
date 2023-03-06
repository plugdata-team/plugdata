---
title: cents2frac

description: cents/fraction conversion

categories:
- object

pdcategory: ELSE, Tuning, Converters

arguments:
- description: conversion resolution
  type: float
  default: 1000, min 10

inlets:
  1st:
  - type: list
    description: cents value(s)

outlets:
  1st:
  - type: list
    description: converted fractional value(s)

methods:
  - type: res <float>
    description: set conversion resolution

draft: false
---

Use [cents2frac] to convert a list of cents to intervals defined as fraction symbols or lists.
