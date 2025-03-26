import shutil
import os
import re
import glob
import sys
import platform
import zipfile
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
    shutil.make_archive(name, "zip", root_dir, base_dir)


def split(a, n):
    k, m = divmod(len(a), n)
    return (a[i*k+min(i, m):(i+1)*k+min(i+1, m)] for i in range(n))


def splitFile(file, output_file, num_files):
    with open(file, 'rb') as fd:
        data_in = split(fd.read(), num_files)
        count = 0
        for entry in data_in:
            with open(output_file.replace("%i", str(count)), "wb") as fd:
                fd.write(entry)
            count += 1


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


def expand_glob_list(file_patterns):
    expanded_files = []
    for pattern in file_patterns:
        expanded_files.extend(glob.glob(pattern))
    return expanded_files


def hash_resource_name(resource_name):
    # Hashes a resource name using the same algorithm as in the C++ code.
    hash_val = 0
    for char in resource_name:
        hash_val = 31 * hash_val + ord(char)
    return hash_val & 0xFFFFFFFF  # Limit to 32-bit hash


def generate_binary_data(output_dir, file_list):
    os.makedirs(output_dir, exist_ok=True)  # Ensure output directory exists
    file_data = []  # To store metadata for each file

    # Gather file information and binary content
    for index, filepath in enumerate(file_list):
        filename = os.path.basename(filepath)
        if os.path.isfile(filepath):
            with open(filepath, "rb") as file:
                binary_content = file.read()

            var_name = filename.replace('.', '_').replace('-', '_')
            temp_name = f"temp_binary_data_{index}"
            data = {
                "filename": filename,
                "var_name": var_name,
                "temp_name": temp_name,
                "size": len(binary_content),
                "binary": list(binary_content),
                "index": str(index),
                "hash": hash_resource_name(var_name)
            }
            file_data.append(data)

    # Generate header file
    with open(output_dir + "/BinaryData.h", "w") as header:
        header.write("namespace BinaryData\n{\n")
        for data in file_data:
            header.write(f"    extern const char* {data['var_name']};\n")
            header.write(f"    const int {data['var_name']}Size = {data['size']};\n")

        header.write("\n    const int namedResourceListSize = {};\n".format(len(file_data)))
        header.write("    extern const char* namedResourceList[];\n")
        header.write("    extern const char* originalFilenames[];\n")
        header.write("    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);\n")
        header.write("}\n")

    with open(output_dir + "/BinaryData.cpp", "w") as source:
        source.write("#include \"BinaryData.h\"\n\n")
        source.write("namespace BinaryData\n{\n\n")

        source.write("\n    const char* namedResourceList[] = {\n")
        for data in file_data:
            source.write(f"        \"{data['var_name']}\",\n")
        source.write("    };\n\n")

        source.write("const char* getNamedResource (const char* resourceNameUTF8, int& numBytes)\n")
        source.write("{\n")
        source.write("    unsigned int hash = 0;\n")
        source.write("    if (resourceNameUTF8 != nullptr)\n")
        source.write("        while (*resourceNameUTF8 != 0)\n")
        source.write("            hash = 31 * hash + (unsigned int) *resourceNameUTF8++;\n\n")

        source.write("    switch (hash)\n    {\n")
        for data in file_data:
            source.write(f"        case 0x{data['hash']:08x}:  numBytes = {data['size']}; return {data['var_name']};\n")
        source.write("        default: break;\n")
        source.write("    }\n\n")

        source.write("    numBytes = 0;\n")
        source.write("    return nullptr;\n")
        source.write("}\n")
        source.write("}\n")

    # Generate .cpp files for each binary resource
    for data in file_data:
        cpp_filename = output_dir + "/BinaryData_" + data["index"] + ".cpp"
        with open(cpp_filename, "w") as cpp_file:
            cpp_file.write("namespace BinaryData\n{\n")
            cpp_file.write(f"//================== {data['filename']} ==================\n")
            cpp_file.write(f"static const unsigned char {data['temp_name']}[] =\n{{\n")

            chunk_size = 64
            for i in range(0, len(data['binary']), chunk_size):
                chunk = data['binary'][i:i + chunk_size]
                # Format and write each chunk without a trailing comma
                hex_values = ','.join(str(byte) for byte in chunk) + ','
                cpp_file.write(f"    {hex_values}\n")

            cpp_file.write("};\n\n")
            cpp_file.write(f"const char* {data['var_name']} = (const char*) {data['temp_name']};\n")
            cpp_file.write("}\n")


if existsAsFile("../Filesystem.zip"):
    removeFile("../Filesystem.zip")

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
convert_merda.process(project_root + "/Libraries/pd-else/Abstractions/Merda/Modules/")
globCopy(project_root + "/Libraries/pd-else/Abstractions/Merda/Modules/*.pd", "./Extra/else")
copyDir(project_root + "/Libraries/pd-else/Abstractions/Merda/Modules/brane-presets", "./Extra/else/brane-presets")
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

splitFile(project_root + "/Resources/Fonts/InterUnicode.ttf", output_dir + "/InterUnicode_%i.ttf", 8)

splitFile("./Filesystem.zip", output_dir + "/Filesystem_%i.zip", 12)
removeFile("./Filesystem.zip")

generate_binary_data("../BinaryData", expand_glob_list({
    project_root + "/Resources/Fonts/IconFont.ttf",
    project_root + "/Resources/Fonts/InterTabular.ttf",
    project_root + "/Resources/Fonts/InterBold.ttf",
    project_root + "/Resources/Fonts/InterSemiBold.ttf",
    project_root + "/Resources/Fonts/InterThin.ttf",
    project_root + "/Resources/Fonts/InterVariable.ttf",
    project_root + "/Resources/Fonts/InterRegular.ttf",
    project_root + "/Resources/Fonts/RobotoMono-Regular.ttf",
    project_root + "/Resources/Icons/plugdata_large_logo.png",
    project_root + "/Resources/Icons/plugdata_logo.png",
    "Documentation.bin",
    "InterUnicode_*.ttf",
    "Filesystem_*.zip"
}))
