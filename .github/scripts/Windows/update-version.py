import plistlib, os, datetime, fileinput, glob, sys, string
scriptpath = "./.github/scripts/Windows/PlugData.iss"

def main():

  version = "v0.6.0"
  print(version)
  
  if len(sys.argv) != 2:
    print("Usage: update_installer_version.py version")
    sys.exit(1)
  else:
    version = str(sys.argv[1])

    print("Updating Windows Installer version info to " + version)
  
    for line in fileinput.input(scriptpath,inplace=1):
        if "AppVersion" in line:
            line="AppVersion=" + version + "\n"
        sys.stdout.write(line)

main()