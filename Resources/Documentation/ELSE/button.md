---

title: button

description: button GUI

categories:
 - object

pdcategory: ELSE, UI

arguments:
inlets:
  1st:
  - type: dim <f, f>
    description: sets horizontal and vertical size in pixels
  - type: size <float>
    description: sets both horizontal and vertical size in pixels
  - type: width <float>
    description: sets horizontal size in pixels
  - type: height <float>
    description: sets vertical size in pixels
  - type: bgcolor <f, f, f>
    description: sets background color in RGB
  - type: fgcolor <f, f, f>
    description: sets foreground color in RGB

outlets:
  1st:
  - type: float
    description: latch status (on=1 or off=0)

flags:
- name: -bng
  description: sets to bang mode (default latch)
- name: -tgl
  description: sets to toggle mode (default latch)
- name: -dim <float, float>
  description: x/y dimensions (default = 20, 20)
- name: size <float>
  description: sets both horizontal and vertical size in pixels
- name: -bgcolor <float, float>
  description: background color in RGB (default 255 255 255)
- name: -fgcolor <float, float>
  description: foreground color in RGB (default 255 255 255)

methods:
  - type: latch
    description: sets to latch mode
  - type: toggle
    description: sets to toggle mode
  - type: bang
    description: sets to bang mode
  - type: dim <f, f>
    description: sets horizontal and vertical size in pixels
  - type: size <float>
    description: sets both horizontal and vertical size in pixels
  - type: width <float>
    description: sets horizontal size in pixels
  - type: height <float>
    description: sets vertical size in pixels
  - type: bgcolor <f, f, f>
    description: sets background color in RGB
  - type: fgcolor <f, f, f>
    description: sets foreground color in RGB

draft: true #you wanna remove them from the inlet?
---

[button] is a GUI button that responds to mouse clicks. When clicked on, it sends a "1" value, and outputs "0" when releasing the mouse button.
