# Converts our json documentation to a compact binary format

import os
import sys
import lzma
import json

def writeInt(stream, n):
    stream += n.to_bytes(4, 'little')

def writeString(stream, s):
    stream += s.encode('utf-8') + b'\x00'

def writeObjectReferenceTable(stream, json_dir):
    for filename in os.listdir(json_dir):
        if not filename.endswith(b".json"):
            continue
        with open(os.path.join(os.fsdecode(json_dir), os.fsdecode(filename)), 'r', encoding='utf-8') as f:
            obj = json.load(f)

        titles = obj.get("title", "")
        if isinstance(titles, str):
            titles = [titles]

        for title in titles:
            writeString(stream, title)
            writeString(stream, obj.get("description", ""))
            writeString(stream, obj.get("origin", ""))

            categories = obj.get("categories", [])
            writeInt(stream, len(categories))
            for cat in categories:
                writeString(stream, cat)

            for key in ("inlets", "outlets"):
                iolets = obj.get(key, [])
                writeInt(stream, len(iolets))
                for iolet in iolets:
                    stream += int(iolet.get("repeat", False)).to_bytes(1, 'little')
                    messages = iolet.get("messages", [])
                    writeInt(stream, len(messages))
                    tooltip = ""
                    for msg in messages:
                        tooltip += "(" + msg.get("type", "") + ") " + msg.get("description", "") + "\n";
                        writeString(stream, msg.get("type", ""))
                        writeString(stream, msg.get("description", ""))
                    writeString(stream, tooltip)

            for key in ("arguments", "methods", "flags"):
                items = obj.get(key, [])
                writeInt(stream, len(items))
                for item in items:
                    writeString(stream, item.get("type", "") or item.get("name", ""))
                    description = item.get("description", "")
                    default = item.get("default", "")
                    if default and not "(default:" in description:
                        description += " (default: " + default + ")"
                    writeString(stream, description)

def parseFilesInDir(dir):
    directory = os.fsencode(dir)
    stream = bytearray()
    for origin in os.listdir(directory):
        originPath = os.path.join(directory, origin)
        if os.path.isdir(originPath):
            writeObjectReferenceTable(stream, originPath)

    output_dir = sys.argv[1]
    with lzma.open(output_dir + "/Documentation.bin", "wb", preset=9) as compressed_file:
        compressed_file.write(stream)

parseFilesInDir("../Documentation")
