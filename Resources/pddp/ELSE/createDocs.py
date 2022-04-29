import math

objects = open('./all_obj')
template = open('./template.md')
txt = template.read()

for object in objects.read().splitlines():
    if object.startswith("else/"):
        object = object.replace("else/", "")

    f = open(object + ".md", "w")
    f.write(txt.replace("$TITLE", object))
    f.close()
