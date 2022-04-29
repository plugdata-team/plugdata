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
- description: sets UDP connection 
  default: TCP
  flag: -u
- description: sets to binary mode 