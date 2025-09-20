---
title: pdlink

description: send messages across network by name

categories:
- object

pdcategory: ELSE, Buffers

arguments:
  - description: address
    type: symbol

inlets:
  1st:
  - type: anything
    description: send message over network
  2nd:
  - type: symbol
    description: set address

outlets:
  1st:
  - type: anything
    description: messages received from network

flags:
  - name: -local
    description: only allow connection from localhost
  - name: -debug
    description: print information about connection status

draft: false
---

[pd.link] allows you to communicate to/from different Pd instances over local network, as different versions and even forks of Pure Data (such as plugdata). It works like [send]/[receive] (or [else/sender]/[else/receiver]) as it just needs a symbol address. It works via network but it's simpler as it doesn't require network or OSC configuration! It also allows you to communicate to a [pd~] subprocess.
