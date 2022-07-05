import plistlib, os, datetime, fileinput, glob, sys, string
scriptpath = "./.github/scripts/Windows/PlugData.iss"

def main():

  version = 0
  
  if len(sys.argv) != 2:
    print("Usage: update_installer_version.py version")
    sys.exit(1)
  else:
    version = int(sys.argv[1])

    print("Updating Windows Installer version info...")
  
    for line in fileinput.input(scriptpath,inplace=1):
        if "AppVersion" in line:
        line="AppVersion=" + str(version) + "\n"

    sys.stdout.write(line)