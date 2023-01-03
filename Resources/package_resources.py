import shutil
import os
import glob


# Utility filesystem functions
def makeArchive(name, root_dir, base_dir):
    shutil.make_archive(name, "zip", root_dir, base_dir)

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

def globFindAndReplaceText(path, to_find, replacement):
    for src in glob.glob(path):
        # Read in the file
        with open(src, 'r', encoding='utf-8') as file :
            filedata = file.read()

        # Replace the target string
        filedata = filedata.replace(to_find, replacement)

        # Write the file out again
        with open(src, 'w', encoding='utf-8') as file:
            file.write(filedata)


if existsAsFile("./Filesystem.zip"):
    removeFile("./Filesystem.zip")

if existsAsDir("./plugdata_version"):
    removeDir("./plugdata_version")

makeDir("plugdata_version")
changeWorkingDir("plugdata_version")

makeDir("Abstractions")
makeDir("Abstractions/else")

copyDir("../../Libraries/pure-data/doc", "./Documentation")
globCopy("../../Libraries/pure-data/extra/*.pd", "./Abstractions")

globCopy("../../Libraries/ELSE/Abstractions/*.pd", "./Abstractions/else")
copyFile("../playhead.pd", "./Abstractions")
copyFile("../param.pd", "./Abstractions")

globMove("./Abstractions/*-help.pd", "./Documentation/5.reference")


copyDir("../pddp", "./Documentation/pddp")
copyDir("../../Libraries/ELSE/Help-files/", "./Documentation/9.else")

copyFile("../../Libraries/ELSE/sfont~/sfont~-help.pd", "./Documentation/9.else")
#copyFile("../param-help.pd", "./Documentation/5.reference")
copyFile("../playhead-help.pd", "./Documentation/5.reference")

globCopy("../../Libraries/cyclone/abstractions/*.pd", "./Abstractions")
copyDir("../../Libraries/cyclone/documentation/help_files", "./Documentation/10.cyclone")
copyDir("../../Libraries/ELSE/Live-Electronics-Tutorial/", "./Documentation/12.live-electronics-tutorial")

makeDir("Documentation/11.heavylib")
copyDir("../../Libraries/heavylib", "./Abstractions/heavylib")
globMove("./Abstractions/heavylib/*-help.pd", "./Documentation/11.heavylib")

# Remove else and cyclone prefixes in helpfiles
globFindAndReplaceText("./Abstractions/else/*.pd", "else/", "")
globFindAndReplaceText("./Documentation/9.else/*.pd", "else/", "")
globFindAndReplaceText("./Documentation/10.cyclone/*.pd", "cyclone/", "")

removeFile("./Documentation/Makefile.am")

# pd-lua
makeDir("Extra")
makeDir("Extra/pdlua")
pdlua_srcdir = "../../../pd-lua/"
for src in ["pd.lua", "COPYING", "README"]:
    copyFile(pdlua_srcdir+src, "./Extra/pdlua")
copyDir(pdlua_srcdir+"doc", "./Extra/pdlua/doc")
makeDir("Documentation/13.pdlua")
for src in ["hello*", "pdlua*-help.pd"]:
    globCopy(pdlua_srcdir+src, "./Documentation/13.pdlua")
for src in ["examples", "tutorial"]:
    copyDir(pdlua_srcdir+src, "./Documentation/13.pdlua/"+src)

changeWorkingDir("./..")

makeArchive("Filesystem", "./", "./plugdata_version")
removeDir("./plugdata_version")


