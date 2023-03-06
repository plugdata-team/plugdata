---
title: thresh

description: combine data received close together

categories:
 - object

pdcategory: cyclone, Data Management

arguments:
- type: float
  description: initial time in ms
  default: 10

inlets:
  1st:
  - type: float/list
    description: to be combined into a list with another input close in time
  2nd:
  - type: float
    description: time interval for combining items to a list

outlets:
  1st:
  - type: float/list
    description: list of elements stored within a time interval

draft: true
---

[thresh] collects numbers and lists into a single list if they come within a certain given amount of time. Each item or list is appended to the previous stored items. The time count is reset at each incoming item.
