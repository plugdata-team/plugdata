---
title: netreceive
description: listen for incoming messages from network
categories:
- object
pdcategory: Misc
last_update: '0.51'
see_also:
- netsend
arguments:
- description: port number
  type: float
- description: UDP hostname or multicast address.
  type: symbol
flags:
- description: sets UDP connection (default TCP).
  flag: -u
- description: sets to binary mode (default 'FUDI').
  flag: -b
- description: flag for from address & port outlet.
  flag: -f
inlets:
  1st:
  - type: listen <float, symbol>
    description: a number sets or changes the port number (0 or negative closes the
      port). Optional symbol is a hostname which can be a UDP multicast address or
      a network interface.
  - type: send <anything>
    description: sends messages back to connected netsend objects.
  - type: list
    description: works like 'send'.
outlets:
  1st:
  - type: anything
    description: messages sent from connected netsend objects.
  2nd:
  - type: float
    description: number of open connections for TCP connections. (TCP connection only)
  rightmost:
  - type: list
    description: address and port. (if the -f flag is given)
draft: false
---
As of 0.51, Pd supports IPv6 addresses.

The netreceive object opens a socket for TCP ("stream") or UDP ("datagram") network reception on a specified port. If using TCP, an outlet gives you the number of netsend objects (or other compatible clients) that have opened connections here.

By default the messages are ASCII text messages compatible with Pd (i.e., numbers and symbols terminated with a semicolon -- the "FUDI" protocol). The "-b" creation argument specifies binary messages instead, which appear in Pd as lists of numbers from 0 to 255 (You could use this for OSC messages, for example.)

There are some possibilities for intercommunication with other programs... see the help for "netsend."

SECURITY NOTE: Don't publish the port number of your netreceive unless you wouldn't mind other people being able to send you messages.

An old (pre-0.45) calling convention is provided for compatibility, port number and following "0" or "1" for TCP or UDP respectively.
