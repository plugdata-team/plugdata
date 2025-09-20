---
title: pdlink~

description: send messages across network by name

categories:
- object

pdcategory: ELSE, Buffers

arguments:
  - description: address
    type: symbol

inlets:
  1st:
  - type: signal
    description: signal to send over network
  2nd:
  - type: symbol
    description: set address

outlets:
  1st:
  - type: anything
    description: received signals

flags:
  - name: -local
    description: only allow connection from localhost
  - name: -debug
    description: print information about connection status
  - name: -compress
    description: apply Opus compression to audio stream
  - name: -bufsize <float>
    description: set buffer size

draft: false
---

[pd.link~] allows you to send/receive audio streans to/from different Pd instances, as different versions, platforms and even forks of Pure Data (such as plugdata). It works like [send~]/[receive~] as it just needs a symbol address. It works via network but it's simpler as it doesn't require network configuration! It also allows you to communicate to a [pd~] subprocess. You can make up to 16 sends to a single receive, but you can receive as many times as you like.
