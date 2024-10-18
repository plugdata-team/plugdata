
#     Tool that can convert our markdown documentation format to either xml or JUCE's ValueTree binary format
#     By doing this at compile time, we can save having to parse all the docs on startup

import os
import sys
import xml.etree.cElementTree as ET

# Write n bytes from a number
def byte(target, n):
  return (target & (0xFF << (8 * n))) >> (8 * n)

# Write a compressed integer, compatible with JUCE's OutputStream
def writeCompressedInt(target, value):
  data = bytearray(5)

  un = (-value + (1 << 32)) if value < 0 else value

  num = 0
  while un > 0:
    num += 1
    data[num] = byte(un, 0)
    un >>= 8
  data[0] = num
  if value < 0:
    data[0] |= 0x80
  #data = data.replace(b"\x0c", b"\x1c")
  target += data[:num+1]

# Write null-terminated UTF8 string
def cString(string):
  return string.encode('utf-8') + b'\x00'

# Write xml object to binary stream, using JUCE's ValueTree binary format
# Only supports string properties because that's all we need
def writeToStream(stream, object):
    attribs = object.attrib;

    # Write tag
    stream += cString(object.tag)
    # Write num attributes
    writeCompressedInt(stream, len(attribs))

    for attr in attribs:
      # Write attribute name
      stream += cString(attr.strip())
      # Get attribute value
      strBytes = cString(object.get(attr).strip())
      # Write length of value +1
      writeCompressedInt(stream, len(strBytes) + 1)
      # Write JUCE::var identifier for string as a single byte
      # Only string properties are allowed because that's all we use
      stream += int(5).to_bytes(1, byteorder='little')
      # Add value to stream
      stream += strBytes

    # Write num children
    writeCompressedInt(stream, len(object))

    # Add all children to stream
    for child in object:
      writeToStream(stream, child)

# Separate markdown by "-"
def sectionsFromHyphens(text):
    lastIdx = 0
    lines = text.splitlines()
    for index, line in enumerate(lines):
      line = line.strip()
      if line.startswith("-"):
        lines[index] = "\n" + line[line.find('-') + 1:].strip()
        lastIdx = index
      else:
        lines[lastIdx] += "\n" + line.strip()
        lines[index] = ""

    while("" in lines):
      lines.remove("")

    return lines

# Separate markdown by a set number of selectors
def getSections(markdown, sectionNames):
  positions = {}

  for section in sectionNames:
    try:
      positions[section] = markdown.index(section + ":")
    except ValueError as ve:
      continue

  positions = dict(sorted(positions.items(), key=lambda item: item[1]))

  sections = {}
  i = 0
  keys = list(positions.keys())
  for i in range(0, len(keys)):
    name = keys[i]
    positionInFile = positions[name] + len(name)
    sectionContent = str()

    if i == len(keys) - 1:
      sectionContent = markdown[positionInFile:]
    else:
      sectionContent = markdown[positionInFile+1:positions[keys[i+1]]]

    sections[name.strip()] = sectionContent[1:].strip().strip('\"')

  return sections


def checkForIllegalChars(strToCheck):
  strToCheck = strToCheck.replace(":", ";")
  strToCheck = strToCheck.replace("- ", " ")

  return strToCheck

def prepareDocsForWebpage(xml):
  mdTitle = "\ntitle: " + xml.get("name");

  mdCategories = ""
  for category in xml.findall("categories")[0]:
    cat = category.get("name").strip()
    if not cat.startswith("ELSE"):
      mdCategories += cat
      break

  indices = ["1st", "2nd", "3rd", "4th", "5th", "6th", "7th", "8th", "nth"];
  mdInlets = ""
  mdOutlets = ""
  numIn = 0;
  numOut = 0;
  for iolet in xml.findall("iolets")[0]:
    if iolet.tag == "inlet":
      idx = indices[numIn]
      numIn += 1
      mdInlets += "\n  " + idx + ":"
    else:
      idx = indices[numOut]
      numOut += 1
      mdOutlets += "\n  " + idx + ":"

    for message in iolet:
      if iolet.tag == "inlet":
        mdInlets += "\n  - type: " + message.get("type") + "\n    description: " + checkForIllegalChars(message.get("description"))
      else:
        mdOutlets += "\n  - type: " + message.get("type") + "\n    description: " + checkForIllegalChars(message.get("description"))

  mdArguments = ""
  for argument in xml.findall("arguments")[0]:
    mdArguments += "\n  - type: " + argument.get("type") + "\n    description: " + checkForIllegalChars(argument.get("description"));

  mdMethods = ""
  for method in xml.findall("methods")[0]:
    mdMethods += "\n  - type: " + method.get("type") + "\n    description: " + checkForIllegalChars(method.get("description"));

  mdFlags = ""
  for flag in xml.findall("flags")[0]:
    mdFlags += "\n  - type: " + flag.get("name") + "\n    description: " + checkForIllegalChars(flag.get("description"));

  if len(mdCategories):
    mdCategories = "pdcategory: " + mdCategories

  if len(mdInlets):
    mdInlets = "inlets:\n" + mdInlets

  if len(mdOutlets):
    mdOutlets = "outlets:\n" + mdOutlets

  if len(mdArguments):
    mdArguments = "arguments:" + mdArguments

  if len(mdMethods):
    mdMethods = "methods:" + mdMethods

  if len(mdFlags):
    mdFlags = "flags:" + mdFlags

  md = "--- \n\n" + mdTitle + "\n\n" + mdCategories + "\n\n" + mdInlets + "\n\n" + mdOutlets + "\n\n" + mdArguments + "\n\n" + mdMethods + "\n\n" + mdFlags + "\n\ndraft: false\n---";

  if " " in xml.get("name"): return


  with open("../Website/" + xml.get("name") + ".md", "w", encoding='utf-8') as markdown:
    # Write bytes to file
    markdown.write(md)
  return

# Converts our markdown docs format to xml
def markdownToXml(root, md):

  sections = getSections(md, {"\ntitle", "\ndescription", "\npdcategory", "\ncategories", "\nflags", "\narguments", "\nlast_update", "\ninlets", "\noutlets", "\ndraft", "\nsee_also", "\nmethods"})

  if "title" not in list(sections.keys()): return

  description = sections["description"] if "description" in list(sections.keys()) else ""

  for title in sections["title"].split(","):
    object = ET.SubElement(root, "object", name=title.strip("\'\"\n\f\r\t "), description=description)
    iolets = ET.SubElement(object, "iolets")
    categories = ET.SubElement(object, "categories")
    arguments = ET.SubElement(object, "arguments")
    methods = ET.SubElement(object, "methods")
    flags = ET.SubElement(object, "flags")
    #print(title)

    if "methods" in sections:
      for section in sectionsFromHyphens(sections["methods"]):
        method = getSections(section, { "name", "type", "description", "default" });
        name = ""
        if "type" in method:
          name = method["type"]
        elif "name" in method:
          name = method["name"]
        else: continue

        ET.SubElement(methods, "method", type=name, description=method["description"])

    if "pdcategory" in sections:
      for section in sections["pdcategory"].split(","):
        ET.SubElement(categories, "category", name=section)

    if "arguments" in sections:
      for argument in sectionsFromHyphens(sections["arguments"]):
        if not len(argument): continue
        sectionMap = getSections(argument, { "type", "description", "default" })
        defaultValue = sectionMap["default"] if "default" in sectionMap else ""
        ET.SubElement(arguments, "argument", type=sectionMap["type"], description=sectionMap["description"], default=defaultValue)

    if "flags" in sections:
      for flag in sectionsFromHyphens(sections["flags"]):
        sectionMap = getSections(flag, { "name", "description", "default" })
        desc = sectionMap["description"] if "description" in sectionMap else ""
        ET.SubElement(flags, "flag", name=sectionMap["name"], description=desc)

    numbers = { "1st", "2nd", "3rd", "4th", "5th", "6th", "7th", "8th", "9th", "10th", "nth" };

    if "inlets" in sections:
      inletSections = getSections(sections["inlets"], numbers)

      for section in list(inletSections.keys()):
        isVariable = str(int(section == "nth"));
        tip = ""
        inlet = ET.Element("inlet", variable=isVariable)
        for argument in sectionsFromHyphens(inletSections[section]):
          typeAndDescription = getSections(argument, { "type", "description" })
          tip += "(" + typeAndDescription["type"] + ") "
          description = ""
          if "description" in typeAndDescription:
           tip += typeAndDescription["description"] + "\n"
           description = typeAndDescription["description"]
          ET.SubElement(inlet, "message", type=typeAndDescription["type"], description=description)
        inlet.set("tooltip", tip.strip())
        iolets.append(inlet)

    if "outlets" in sections:
      outletSections = getSections(sections["outlets"], numbers)

      for section in list(outletSections.keys()):
        isVariable = str(int(section == "nth"));
        tip = ""
        outlet = ET.Element("outlet", variable=isVariable)
        for argument in sectionsFromHyphens(outletSections[section]):
          typeAndDescription = getSections(argument, { "type", "description" })
          tip += "(" + typeAndDescription["type"] + ") "
          description = ""
          if "description" in typeAndDescription:
           tip += typeAndDescription["description"] + "\n"
           description = typeAndDescription["description"]
          ET.SubElement(outlet, "message", type=typeAndDescription["type"], description=description)
        outlet.set("tooltip", tip.strip())
        iolets.append(outlet)

# Iterate over markdown files in search dirs
def parseFilesInDir(dir, generateXml, generateWebsite):
  directory = os.fsencode(dir)
  root = ET.Element("root")
  # Find files
  for origin in os.listdir(directory):
    originPath = os.path.join(directory, origin)
    if(os.path.isdir(originPath)):
      for file in os.listdir(originPath):
        filePath = os.path.join(originPath, file)
        if os.fsdecode(filePath).endswith(".md"):
          with open(filePath, 'r', encoding='utf-8') as f:
              markdownToXml(root, f.read())

  # Save the generate xml if asked for
  if generateXml:
    tree = ET.ElementTree(root)
    tree.write("../Documentation/Documentation.xml")

  # Convert xml to JUCE ValueTree binary format
  stream = bytearray()
  writeToStream(stream, root)

  if generateWebsite:
    for child in root:
      prepareDocsForWebpage(child)

  output_dir = sys.argv[1]
  with open(output_dir + "/Documentation.bin", "wb") as binaryFile:
    # Write bytes to file
    binaryFile.write(stream)

parseFilesInDir("../Documentation", False, False)
