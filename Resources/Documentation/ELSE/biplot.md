---
title: biplot

description: biquad plot

categories:
- object

pdcategory: ELSE, Filters, Analysis

arguments:
inlets:
  1st:
  - type: list
    description: list of coefficients as taken by [biquad~]

outlets:
  1st:
  - type: list
    description: list of coefficients is passed through

draft: false
---

[biplot] plots the filter response from biquad coefficients, the ones you send to [biquad~] or [biquads~].

