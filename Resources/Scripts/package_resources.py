import shutil
import os
import re
import glob
import sys
import platform
import lzma
import zipfile
import tarfile

import convert_merda

# Parse arguments
value_mappings = {
    "0": False,
    "1": True,
    "ON": True,
    "OFF": False,
    "TRUE": True,
    "FALSE": False
}

package_gem = value_mappings[sys.argv[1].upper()]
output_dir = sys.argv[2]

# Utility filesystem functions


def removeFile(path):
    os.remove(path)


def removeDir(path):
    shutil.rmtree(path)


def makeDir(path):
    os.mkdir(path)


def changeWorkingDir(path):
    os.chdir(path)


def copyFile(src, dst):
    shutil.copy(src, dst)


def moveFile(src, dst):
    shutil.copy(src, dst)


def copyDir(src, dst):
    # Prepend paths with \\?\ to deal with file names >260 chars in ELSE and DOS
    if (platform.system().lower() == "windows"):
        src = '\\\\?\\' + os.path.abspath(src)
        dst = '\\\\?\\' + os.path.abspath(dst)
    shutil.copytree(src, dst)


def globCopy(srcs, dst):
    for src in glob.glob(srcs):
        shutil.copy(src, dst)


def globMove(srcs, dst):
    for src in glob.glob(srcs):
        shutil.move(src, dst)


def existsAsFile(path):
    return os.path.isfile(path)


def existsAsDir(path):
    return os.path.isdir(path)

def makeArchive(name, root_dir, base_dir):
    archive_path = os.path.abspath(name)
    preset = 9 | lzma.PRESET_EXTREME  # max compression
    with lzma.open(archive_path, "wb", preset=preset) as xz_out:
        with tarfile.open(fileobj=xz_out, mode="w|") as tar:
            full_path = os.path.join(root_dir, base_dir)
            tar.add(full_path, arcname=base_dir)

def replaceTextInFile(file_path, old_string, new_string):
    with open(file_path, 'r', encoding='utf-8') as file:
        content = file.read()

    content_new = re.sub(re.escape(old_string), new_string, content)

    with open(file_path, 'w', encoding='utf-8') as file:
        file.write(content_new)


def replaceTextInFolder(folder_path, old_string, new_string):
    for root, dirs, files in os.walk(folder_path):
        for file_name in files:
            if file_name.endswith('.pd'):
                file_path = os.path.join(root, file_name)
                replaceTextInFile(file_path, old_string, new_string)
        for dir_name in dirs:
            replaceTextInFolder(os.path.join(
                root, dir_name), old_string, new_string)

if existsAsFile("../Filesystem"):
    removeFile("../Filesystem")

if existsAsDir(output_dir + "/plugdata_version"):
    removeDir(output_dir + "/plugdata_version")

project_root = os.path.dirname(os.path.realpath(os.getcwd() + "/.."))
makeDir(output_dir + "/plugdata_version")
changeWorkingDir(output_dir + "/plugdata_version")

makeDir("Abstractions")
makeDir("Abstractions/else")
makeDir("Abstractions/cyclone")

copyDir(project_root + "/Libraries/pure-data/doc", "./Documentation")
globCopy(project_root + "/Libraries/pure-data/extra/*.pd", "./Abstractions")
globCopy(project_root + "/Libraries/pure-data/extra/**/*-help.pd", "./Abstractions")

globCopy(project_root + "/Libraries/pd-else/Abstractions/Control/*.pd", "./Abstractions/else")
globCopy(project_root + "/Libraries/pd-else/Abstractions/Audio/*.pd", "./Abstractions/else")
globCopy(project_root + "/Libraries/pd-else/Abstractions/Extra/*.pd", "./Abstractions/else")
globCopy(project_root + "/Libraries/pd-else/Source/Control/*.pd_lua", "./Abstractions/else")
globCopy(project_root + "/Libraries/pd-else/Source/Audio/*.pd_lua", "./Abstractions/else")
globCopy(project_root + "/Libraries/pd-else/Abstractions/Extra/*.pd_lua", "./Abstractions/else")
copyFile(project_root + "/Resources/Patches/lua.pd_lua", "./Abstractions/else")
copyFile(project_root + "/Resources/Patches/playhead.pd", "./Abstractions")
copyFile(project_root + "/Resources/Patches/param.pd", "./Abstractions")
copyFile(project_root + "/Resources/Patches/daw_storage.pd", "./Abstractions")
copyFile(project_root + "/Resources/Patches/plugin_latency.pd", "./Abstractions")
# copyFile("../../Patches/beat.pd", "./Abstractions")

globMove("Abstractions/*-help.pd", "./Documentation/5.reference")
copyDir(project_root + "/Libraries/pd-else/Documentation/Help-files/", "./Documentation/9.else")
copyFile(project_root + "/Libraries/pd-else/Documentation/Extra-files/f2s~-help.pd", "./Documentation/9.else")

# copyFile("../../Patches/beat-help.pd", "./Documentation/5.reference")
copyFile(project_root + "/Resources/Patches/param-help.pd", "./Documentation/5.reference")
copyFile(project_root + "/Resources/Patches/playhead-help.pd", "./Documentation/5.reference")
copyFile(project_root + "/Resources/Patches/daw_storage-help.pd", "./Documentation/5.reference")
copyFile(project_root + "/Resources/Patches/plugin_latency-help.pd", "./Documentation/5.reference")

globCopy(project_root + "/Libraries/pd-cyclone/cyclone_objects/abstractions/*.pd", "./Abstractions/cyclone")
copyDir(project_root + "/Libraries/pd-cyclone/documentation/help_files", "./Documentation/10.cyclone")
globCopy(project_root + "/Libraries/pd-cyclone/documentation/extra_files/*", "./Documentation/10.cyclone/")
moveFile(project_root + "/Libraries/pd-cyclone/documentation/extra_files/All_about_cyclone.pd", "./Abstractions/cyclone/")  # help files want to find this here
moveFile("./Documentation/10.cyclone/dsponoff~.pd", "./Abstractions/cyclone/dsponoff~.pd")
copyDir(project_root + "/Libraries/pd-else/Documentation/Live-Electronics-Tutorial/", "./Documentation/12.live-electronics-tutorial")

makeDir("Documentation/11.heavylib")
copyDir(project_root + "/Libraries/heavylib", "./Abstractions/heavylib")
globMove("./Abstractions/heavylib/*-help.pd", "./Documentation/11.heavylib")

removeFile("Documentation/Makefile.am")

makeDir("Extra")
copyDir(project_root + "/Libraries/pd-else/Documentation/Extra-files", "Extra/else")
globCopy("Extra/else/audio/*", "Extra/else")
removeDir("Extra/else/audio")

copyFile(project_root + "/Libraries/pd-else/Documentation/README.pdf", "Extra/else")
copyDir(project_root + "/Libraries/pd-else/Source/Audio/sfz~/sfz", "Extra/else/sfz")
copyDir(project_root + "/Resources/Patches/Presets", "./Extra/Presets")

convert_merda.process(project_root + "/Libraries/pd-else/Abstractions/Merda", output_dir + "/Merda_temp")

globCopy(output_dir + "/Merda_temp/*", "./Extra/else")
copyDir(project_root + "/Libraries/pd-else/Abstractions/Merda/Modules/brane-presets", "./Extra/else/brane-presets")
removeDir(output_dir + "/Merda_temp")

globCopy(project_root + "/Libraries/pure-data/doc/sound/*", "Extra/else")

# Our folder is called "Documentation" instead of "doc", which makes some file paths in default helpfiles invalid
replaceTextInFolder("Documentation/5.reference", "../doc/5.reference/", "../Documentation/5.reference/")
replaceTextInFolder("Documentation/5.reference", "../doc/sound/", "../Documentation/sound/")
replaceTextInFolder("Abstractions/cyclone", "All_objects", "All_cyclone_objects")

# pd-lua
makeDir("Extra/pdlua")

pdlua_srcdir = project_root + "/Libraries/pd-lua/"
for src in ["pd.lua", "COPYING", "README", "pdlua/tutorial/examples/pdx.lua"]:
    copyFile(pdlua_srcdir+src, "./Extra/pdlua")
# These are developer docs, we don't need them.
# copyDir(pdlua_srcdir+"doc", "./Extra/pdlua/doc")
makeDir("Documentation/13.pdlua")
for src in ["pdlua*-help.pd"]:
    globCopy(pdlua_srcdir+src, "./Documentation/13.pdlua")
for src in ["pdlua"]:
    copyDir(pdlua_srcdir+src, "./Documentation/13.pdlua/"+src)

if package_gem:
    makeDir("Abstractions/Gem")

    copyDir(project_root + "/Libraries/Gem/help", "Documentation/14.gem")
    copyDir(project_root + "/Libraries/Gem/examples", "Documentation/14.gem/examples")
    copyDir(project_root + "/Libraries/Gem/doc", "Documentation/14.gem/examples/Documentation")
    globCopy(project_root + "/Libraries/Gem/abstractions/*.pd", "Abstractions/Gem/")
    globMove("Abstractions/Gem/*-help.pd", "Documentation/14.gem/")

    makeDir("Extra/Gem")  # user can put plugins and resources in here

    # extract precompiled Gem plugins for our architecture
    system = platform.system().lower()
    architecture = platform.architecture()
    machine = platform.machine()

changeWorkingDir("../")

makeArchive("Filesystem", "./", "./plugdata_version")

removeDir(output_dir + "/plugdata_version")

def generateBinaryDataArchive(output_dir, file_list):
    """
    Generate a binary resource archive with a C++ header/implementation.

    Creates:
    - plugdata-resources.bin: Binary archive containing all resources
    - BinaryData.h: Header with extern declarations
    - BinaryData.cpp: Implementation with resource loading
    """
    os.makedirs(output_dir, exist_ok=True)

    archive_data = bytearray()
    file_index = []

    for filepath in file_list:
        if os.path.isfile(filepath):
            var_name = os.path.basename(filepath).replace('.', '_').replace('-', '_')

            with open(filepath, "rb") as f:
                content = f.read()

            offset = len(archive_data)
            size = len(content)
            archive_data.extend(content)

            file_index.append({
                'var_name': var_name,
                'offset': offset,
                'size': size
            })

    # Write the data archive
    with open(os.path.join(output_dir, "plugdata-resources.bin"), "wb") as f:
        f.write(archive_data)

    # Generate single header file with inline implementation
    with open(os.path.join(output_dir, "BinaryData.h"), "w") as header:
        header.write("#pragma once\n\n")
        header.write("#include <cstdint>\n")
        header.write("#include <vector>\n")
        header.write("#include <cstring>\n")
        header.write("#include <juce_core/juce_core.h>\n\n")
        header.write("#include <juce_audio_processors/juce_audio_processors.h>\n\n")
        header.write("namespace BinaryData {\n\n")

        # Generate enum
        header.write("enum Resource {\n")
        for i, data in enumerate(file_index):
            header.write(f"    {data['var_name']}")
            if i < len(file_index) - 1:
                header.write(",\n")
            else:
                header.write("\n")
        header.write("};\n\n")

        header.write(f"constexpr int namedResourceListSize = {len(file_index)};\n\n")

        header.write("\nstruct ResourceInfo {\n")
        header.write("    uint32_t offset;\n")
        header.write("    uint32_t size;\n")
        header.write("};\n\n")

        # Resource table
        header.write(f"static constexpr ResourceInfo resources[] = {{\n")
        for data in file_index:
            header.write(f"    {{ {data['offset']}, {data['size']}}},\n")
        header.write("};\n\n")

        header.write("inline juce::File getResourceFile() {\n")
        header.write("#if JUCE_MAC\n")
        header.write("    if(juce::PluginHostType::getPluginLoadedAs() != juce::AudioProcessor::wrapperType_LV2)\n")
        header.write("        return juce::File::getSpecialLocation(juce::File::currentExecutableFile)\n")
        header.write("            .getParentDirectory().getParentDirectory()\n")  # Up to Contents/
        header.write("            .getChildFile(\"Resources/plugdata-resources.bin\");\n")
        header.write("#endif\n")
        header.write("    return juce::File::getSpecialLocation(juce::File::currentExecutableFile)\n")
        header.write("        .getParentDirectory()\n")
        header.write("        .getChildFile(\"plugdata-resources.bin\");\n")

        header.write("}\n\n")

        header.write("inline std::vector<char> getResource(Resource resource) {\n")
        header.write("    auto resourceFile = getResourceFile();\n")
        header.write("    if (!resourceFile.existsAsFile()) {\n")
        header.write("        jassertfalse; // could not load resource file! \n")
        header.write("        return {};\n")
        header.write("    }\n\n")
        header.write("    const auto& r = resources[resource];\n")
        header.write("    std::vector<char> data(r.size);\n\n")
        header.write("    juce::FileInputStream stream(resourceFile);\n")
        header.write("    if (stream.openedOk()) {\n")
        header.write("        stream.setPosition(r.offset);\n")
        header.write("        stream.read(data.data(), r.size);\n")
        header.write("    }\n\n")
        header.write("    return data;\n")
        header.write("}\n\n")

        header.write("inline int getResourceSize(Resource resource) {\n")
        header.write("    return resources[resource].size; \n")
        header.write("} \n")

        header.write("inline std::unique_ptr<juce::FileInputStream> createInputStream(Resource resource) {\n")
        header.write("    auto resourceFile = getResourceFile();\n")
        header.write("    if (!resourceFile.existsAsFile())\n")
        header.write("        return nullptr;\n\n")
        header.write("    auto stream = std::make_unique<juce::FileInputStream>(resourceFile);\n")
        header.write("    if (!stream->openedOk())\n")
        header.write("        return nullptr;\n\n")
        header.write("    const auto& r = resources[resource];\n")
        header.write("    stream->setPosition(r.offset);\n")
        header.write("    // Note: caller should only read up to r.size bytes\n")
        header.write("    return stream;\n")
        header.write("}\n\n")

        header.write("inline unsigned char* getResourceCopy(Resource resource) {\n")
        header.write("    auto data = getResource(resource);\n")
        header.write("    if (data.empty()) \n")
        header.write("        return nullptr;\n")
        header.write("    auto* copy = (unsigned char*)malloc(data.size());\n")
        header.write("    std::memcpy(copy, data.data(), data.size());\n")
        header.write("    return copy;\n")
        header.write("}\n\n")

        header.write("inline juce::Image loadImage(Resource resource) {\n")
        header.write("    auto image = getResource(resource);\n")
        header.write("    return juce::ImageFileFormat::loadFrom(image.data(), image.size());\n")
        header.write("}\n\n")

        header.write("inline juce::Typeface::Ptr loadFont(Resource resource) {\n")
        header.write("    auto font = getResource(resource);\n")
        header.write("    return juce::Typeface::createSystemTypefaceFor(font.data(), font.size());\n")
        header.write("}\n\n")
        header.write("} // namespace BinaryData\n")


resources = [
    project_root + "/Resources/Fonts/IconFont.ttf",
    project_root + "/Resources/Fonts/InterTabular.ttf",
    project_root + "/Resources/Fonts/InterBold.ttf",
    project_root + "/Resources/Fonts/InterSemiBold.ttf",
    project_root + "/Resources/Fonts/InterVariable.ttf",
    project_root + "/Resources/Fonts/InterRegular.ttf",
    project_root + "/Resources/Fonts/RobotoMono-Regular.ttf",
    project_root + "/Resources/Fonts/RobotoMono-Bold.ttf",
    project_root + "/Resources/Icons/plugdata_large_logo.png",
    project_root + "/Resources/Icons/plugdata_logo.png",
    "Documentation.bin",
    "Filesystem"
]

if platform.system().lower() == "linux" or "bsd" in platform.system().lower():
    with zipfile.ZipFile(output_dir + "/InterUnicode.ttf.zip", 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zipf:
        zipf.write(project_root + "/Resources/Fonts/InterUnicode.ttf", arcname="InterUnicode.ttf")
    resources.append("InterUnicode.ttf.zip")

generateBinaryDataArchive("../BinaryData", resources)
