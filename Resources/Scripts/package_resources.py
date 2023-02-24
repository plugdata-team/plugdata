import shutil
import os
import glob
import sys

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

def copyDir(src, dst):
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

def splitFile(file, num_files):
  with open(file, 'rb') as fd:
    data_in = split(fd.read(), num_files)
    count = 0;
    for entry in data_in:
      name = os.path.splitext(file)[0];
      extension = os.path.splitext(file)[1];
      filename = name + "_" + str(count) + extension
      with open(filename, "wb") as fd:
        fd.write(entry)
      count += 1

if existsAsFile("../Filesystem.zip"):
    removeFile("../Filesystem.zip")

if existsAsDir("../plugdata_version"):
    removeDir("../plugdata_version")

makeDir("../plugdata_version")
changeWorkingDir("../plugdata_version")

makeDir("Abstractions")
makeDir("Abstractions/else")

copyDir("../../Libraries/pure-data/doc", "./Documentation")
globCopy("../../Libraries/pure-data/extra/*.pd", "./Abstractions")
globCopy("../../Libraries/pure-data/extra/**/*-help.pd", "./Abstractions")

globCopy("../../Libraries/ELSE/Abstractions/*.pd", "./Abstractions/else")
copyFile("../Patches/playhead.pd", "./Abstractions")
copyFile("../Patches/param.pd", "./Abstractions")
#copyFile("../Patches/beat.pd", "./Abstractions")

globMove("./Abstractions/*-help.pd", "./Documentation/5.reference")

copyDir("../Documentation", "./Documentation/pddp")
copyDir("../../Libraries/ELSE/Help-files/", "./Documentation/9.else")

copyFile("../../Libraries/ELSE/sfont~/sfont~-help.pd", "./Documentation/9.else")
#copyFile("../Patches/beat-help.pd", "./Documentation/5.reference")
copyFile("../Patches/param-help.pd", "./Documentation/5.reference")
copyFile("../Patches/playhead-help.pd", "./Documentation/5.reference")

globCopy("../../Libraries/cyclone/cyclone_objects/abstractions/*.pd", "./Abstractions")
copyDir("../../Libraries/cyclone/documentation/help_files", "./Documentation/10.cyclone")
copyDir("../../Libraries/ELSE/Live-Electronics-Tutorial/", "./Documentation/12.live-electronics-tutorial")

makeDir("Documentation/11.heavylib")
copyDir("../../Libraries/heavylib", "./Abstractions/heavylib")
globMove("./Abstractions/heavylib/*-help.pd", "./Documentation/11.heavylib")

removeFile("./Documentation/Makefile.am")

# pd-lua
makeDir("Extra")
makeDir("Extra/pdlua")
makeDir("Extra/GS")

copyDir("../../Libraries/ELSE/Extra", "Extra/else");
copyDir("../../Libraries/ELSE/sfont~/sf", "Extra/else/sf");
globCopy("../../Libraries/pure-data/doc/sound/*", "Extra/else");

pdlua_srcdir = "../../Libraries/pd-lua/"
for src in ["pd.lua", "COPYING", "README"]:
    copyFile(pdlua_srcdir+src, "./Extra/pdlua")

# These are developer docs, we don't need them.
#copyDir(pdlua_srcdir+"doc", "./Extra/pdlua/doc")

# The remaining example patches go into 9.else which already has the
# ELSE branded toplevel pdlua and pdluax help patches. NOTE: Once these
# are also included in the ELSE source, they can be taken from there.
for src in ["pdlua"]:
    copyDir(pdlua_srcdir+src, "./Documentation/9.else/"+src)

changeWorkingDir("./..")

makeArchive("Filesystem", "./", "./plugdata_version")
removeDir("./plugdata_version")

splitFile("./Fonts/InterUnicode.ttf", 12)

splitFile("./Filesystem.zip", 12)
removeFile("./Filesystem.zip")
