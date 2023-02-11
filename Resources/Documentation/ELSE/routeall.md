---
title: routeall

description: route all messages

categories:
 - object

pdcategory: ELSE, Data Management

arguments:
- type: anything
  description: arguments to match the 1st element of a message
  default:

inlets:
  1st:
  - type: anything
    description: any message to be routed completely
  2nd:
  - type: float
    description: set element number to match (default 0)

outlets:
  nth:
  - type: anything
    description: if an input message matches an argument, the corresponding outlet sends that message
  2nd:
  - type: anything
    description: any unmatched message

draft: false
---

[routeall] routes messages and - unlike [route] - keeps its first element! Also differently from [route], the [routeall] object matches a symbol to a symbol or a list message! Another difference is that [routeall] takes both float and symbol messages at once! Moreover, it can match to other elements in the message than the first.
Each argument has its corresponding outlet. The object searches for a match between the first element of a message and an argument (from first to last). As soon as a match is found, it sends the message to the corresponding output - this means that it stops looking for other matches. If no match is found, the message is sent to the rightmost outlet.
