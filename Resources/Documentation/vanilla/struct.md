---
title: struct
description: declare the fields in a data structure
categories:
- object
see_also: 
- drawpolygon
- drawtext
- plot
pdcategory: vanilla, Data Structures
last_update: '0.35'
outlets:
  1st:
  - type: anything
    description: messages notifying when there are interactions with objects of the structure ('select', 'deselect', 'click', 'displace' and 'change')
arguments:
- type: list
  description: template name plus types and names of given fields (array fields also need the array's template name)
draft: false
---
There should be one "struct" object in each Pd window you are using as a data structure template. The arguments specify the types and names of the fields. For array fields, a third argument specifies the template that the array elements should belong to

