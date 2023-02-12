---
title: declare
description: set environment for loading patch
categories:
- object
pdcategory: vanilla, UI, File Management
last_update: '0.52'
flags:
- name: -path <symbol>
  description: add to search path, relative to the patch or user paths
- name: -stdpath <symbol>
  description: add to search path, relative to Pd (the 'extra' folder)
- name: -lib <symbol>
  description: load a library, relative to the patch or user paths
- name: -stdlib <symbol>
  description: load a library, relative to Pd (the 'extra' folder)
draft: false
---
Compiled external libraries come either as a single binary pack (the "classic" library format) or as a set of separate binaries and/or abstractions. A single binary pack is what we refer to as a 'library' and needs to be pre loaded - whereas external libraries that have separate binaries/abstractions can be loaded by simply adding its directory to the search path. Adding a directory to the path is also needed if you want to load things like audio and text files that are in it.

A declare object adds one or more directories to the search path and/or pre-loads one or more libraries ("externals") to Pd in preparation for opening the patch from a file. The usage is "declare [-flag value] [-flag value] ..." (For any of these you may use a full pathname such as "/tmp/dir" or, in Windows, "C:/garbage" instead of a relative path).

For instance, if you put abstractions and/or other supporting files in a subdirectory "more", you can put an object "declare -path more" to make sure Pd sees them when the patch is loaded. Or, if you have files installed in the directory extra/stillmore (in the Pd installation) you can get it using "declare -stdpath stillmore".

Paths declared with '-path' will have top search priority. Next priority is the relative path, then user added search paths (set in "Preferences => Path") and finally the standard path (a.k.a the 'extra' folder). As of version 0.49, "declare -path" and "declare -lib" will fall back the other search paths if the relative path to the patch does not exist. To avoid checking further, use an explicit relative path by prepending "./" or "../" to the path or lib name.

Since the 'extra' folder is the last in the search priority, other paths are searched before. You can use [declare -stdpath ./] to ensure that 'extra' has search priority. Note that the order you specify '-path' flags also specify the search priority.

It is a current best practice to just use [declare] instead of permanently adding paths to the user search paths or libs to startup. With [declare] you can better manage and avoid conflicts with externals from different libraries that have the same name by using it to call the right library you want.

However, note that when you load a library (with 'declare -lib' or via startup), all of its objects get pre loaded and prevail, so using 'declare -path' cannot enforce loading priority. Hence, you may need to adopt slash declarations as in [library/objectname]. Also Note that while '-path' will only add search paths for the patch that owns the [declare] object, once a library is loaded, it stays with Pd and will be able to be loaded in other patches without [declare]. For more details on this and how external loading works in Pd, please refer to the chapter 4 of Pd's manual.

WARNING: as of version 0.47, "declare -path" and "declare -stdpath" inside abstractions take effect only within those abstractions. If Pd's compatibility version is set to 0.46 or earlier the old (buggy) behavior takes effect.

BUG: The name "-stdpath" is confusing, as it has a quite different effect from "-stdpath" on the pd command line.
