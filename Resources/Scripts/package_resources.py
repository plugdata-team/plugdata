import shutil
import os
import re
import glob
import sys
import platform
import zipfile

#Parse arguments
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
            replaceTextInFolder(os.path.join(root, dir_name), old_string, new_string)


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

globCopy(project_root + "/Libraries/pd-else/Code_source/Abstractions/control/*.pd", "./Abstractions/else")
globCopy(project_root + "/Libraries/pd-else/Code_source/Abstractions/audio/*.pd", "./Abstractions/else")
globCopy(project_root + "/Libraries/pd-else/Code_source/Abstractions/extra_abs/*.pd", "./Abstractions/else")
globCopy(project_root + "/Libraries/pd-else/Code_source/Compiled/control/*.pd_lua", "./Abstractions/else")
globCopy(project_root + "/Libraries/pd-else/Code_source/Compiled/audio/*.pd_lua", "./Abstractions/else")
globCopy(project_root + "/Libraries/pd-else/Code_source/Abstractions/extra_abs/*.pd_lua", "./Abstractions/else")
copyFile(project_root + "/Resources/Patches/lua.pd_lua", "./Abstractions/else")
copyFile(project_root + "/Resources/Patches/playhead.pd", "./Abstractions")
copyFile(project_root + "/Resources/Patches/param.pd", "./Abstractions")
copyFile(project_root + "/Resources/Patches/daw_storage.pd", "./Abstractions")
copyFile(project_root + "/Resources/Patches/plugin_latency.pd", "./Abstractions")
#copyFile("../../Patches/beat.pd", "./Abstractions")

globMove("Abstractions/*-help.pd", "./Documentation/5.reference")
copyDir(project_root + "/Libraries/pd-else/Documentation/Help-files/", "./Documentation/9.else")
copyFile(project_root + "/Libraries/pd-else/Documentation/extra_files/f2s~-help.pd", "./Documentation/9.else")

#copyFile("../../Patches/beat-help.pd", "./Documentation/5.reference")
copyFile(project_root + "/Resources/Patches/param-help.pd", "./Documentation/5.reference")
copyFile(project_root + "/Resources/Patches/playhead-help.pd", "./Documentation/5.reference")
copyFile(project_root + "/Resources/Patches/daw_storage-help.pd", "./Documentation/5.reference")
copyFile(project_root + "/Resources/Patches/plugin_latency-help.pd", "./Documentation/5.reference")

globCopy(project_root + "/Libraries/pd-cyclone/cyclone_objects/abstractions/*.pd", "./Abstractions/cyclone")
copyDir(project_root + "/Libraries/pd-cyclone/documentation/help_files", "./Documentation/10.cyclone")
globCopy(project_root + "/Libraries/pd-cyclone/documentation/extra_files/*", "./Documentation/10.cyclone/")
moveFile(project_root + "/Libraries/pd-cyclone/documentation/extra_files/All_about_cyclone.pd", "./Abstractions/cyclone/") # help files want to find this here
moveFile("./Documentation/10.cyclone/dsponoff~.pd", "./Abstractions/cyclone/dsponoff~.pd")
copyDir(project_root + "/Libraries/pd-else/Documentation/Live-Electronics-Tutorial/", "./Documentation/12.live-electronics-tutorial")

makeDir("Documentation/11.heavylib")
copyDir(project_root + "/Libraries/heavylib", "./Abstractions/heavylib")
globMove("./Abstractions/heavylib/*-help.pd", "./Documentation/11.heavylib")

removeFile("Documentation/Makefile.am")

makeDir("Extra")
makeDir("Extra/GS")
copyDir(project_root + "/Libraries/pd-else/Documentation/extra_files", "Extra/else")
copyFile(project_root + "/Libraries/pd-else/Documentation/README.pdf", "Extra/else")
copyFile(project_root + "/Libraries/pd-else/Code_source/Merda/Modules/about.MERDA.pd", "./Extra/else")
copyDir(project_root + "/Libraries/pd-else/Code_source/Compiled/audio/sfont~/sf", "Extra/else/sf")
copyDir(project_root + "/Libraries/pd-else/Code_source/Compiled/audio/sfz~/sfz", "Extra/else/sfz")
copyDir(project_root + "/Resources/Patches/Presets", "./Extra/Presets")
copyDir(project_root + "/Resources/Patches/Palettes", "./Extra/palette")
globCopy(project_root + "/Libraries/pure-data/doc/sound/*", "Extra/else")

# Our folder is called "Documentation" instead of "doc", which makes some file paths in default helpfiles invalid
replaceTextInFolder("Documentation/5.reference", "../doc/5.reference/", "../Documentation/5.reference/")
replaceTextInFolder("Documentation/5.reference", "../doc/sound/", "../Documentation/sound/")
replaceTextInFolder("Abstractions/cyclone", "All_objects", "All_cyclone_objects")

# pd-lua
makeDir("Extra/pdlua")

pdlua_srcdir = project_root + "/Libraries/pd-lua/"
for src in ["pd.lua", "COPYING", "README"]:
    copyFile(pdlua_srcdir+src, "./Extra/pdlua")
# These are developer docs, we don't need them.
#copyDir(pdlua_srcdir+"doc", "./Extra/pdlua/doc")
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

    makeDir("Extra/Gem") # user can put plugins and resources in here

    # extract precompiled Gem plugins for our architecture
    system = platform.system().lower()
    architecture = platform.architecture()
    machine = platform.machine()

    gem_plugin_path = project_root + "/Libraries/Gem/"
    gem_plugins_file = ""

    if system == 'linux':
        if 'aarch64' in machine or 'arm' in machine:
            gem_plugins_file = 'plugins_linux_arm64'
        elif '64' in machine:
            gem_plugins_file = 'plugins_linux_x64'
    elif system == 'darwin':
        gem_plugins_file = 'plugins_macos'
    elif system == 'windows' and '64' in architecture[0]:
        gem_plugins_file = 'plugins_win64'

    # unpack if architecture is supported
    if len(gem_plugins_file) != 0:
        with zipfile.ZipFile(gem_plugin_path + gem_plugins_file + ".zip", 'r') as zip_ref:
                zip_ref.extractall("../Extra/Gem/")
                globMove("../Extra/Gem/" + gem_plugins_file + "/*", "Extra/Gem/")
                removeDir("../Extra/Gem/" + gem_plugins_file)

changeWorkingDir("../")

makeArchive("Filesystem", "./", "./plugdata_version")
removeDir(output_dir + "/plugdata_version")

splitFile(project_root + "/Resources/Fonts/InterUnicode.ttf", output_dir + "/InterUnicode_%i.ttf", 8)

splitFile("./Filesystem.zip", output_dir + "/Filesystem_%i.zip", 12)
removeFile("./Filesystem.zip")
