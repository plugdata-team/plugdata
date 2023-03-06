---
title: netreceive
description: listen for incoming messages from network
categories:
- object
pdcategory: vanilla, Networking
last_update: '0.51'
see_also:
- netsend
arguments:
- description: port number
  type: float
- description: UDP hostname or multicast address
  type: symbol
flags:
- name: -u
  description: sets UDP connection (default: TCP)
- name: -b
  description: sets to binary mode (default: FUDI)
- name: -f
  description: flag for from address & port outlet
inlets:
  1st:
  - type: list
    description: works like 'send'
outlets:
  1st:
  - type: anything
    description: messages sent from connected netsend objects
  2nd:
  - type: float
    description: number of open connections for TCP connections (TCP only)
  3rd: #wrong! if -u -f are given this should be the second (rightmost)
  - type: list
    description: address and port (if -f flag is given)

methods:
  - type: listen <float, symbol>
    description: a number sets or changes the port number (0 or negative closes the port). Optional symbol is a hostname which can be a UDP multicast address or a network interface
  - type: send <anything>
    description: sends messages back to connected netsend objects

draft:true
---

