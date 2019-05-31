import os
import platform
import sys

def check_dir(dir):
    if(not os.path.exists(dir)):
        os.makedirs(dir)

def get_qt_version(dir):
  with open(os.path.join(dir, "mkspecs", "qconfig.pri"), 'r') as f:
    lines = f.read().replace("\r","").split("\n")
    for line in lines:
      vars = line.split(" ")
      if len(vars) == 3 and vars[0] == "QT_VERSION":
        return "QT"+vars[2]
  return "Unknown"

if platform.system() == "Windows":
    root = os.getcwd()
      
    if "QTDIR32" in os.environ:
      tag = get_qt_version(os.environ["QTDIR32"])
      build_dir = os.path.join(os.getcwd(), "build", platform.system(), "x86_" + tag)
      bin_dir = os.path.join(os.getcwd(), "bin", platform.system(), "x86_" + tag, "Release")
      lib_dir = os.path.join(os.getcwd(), "lib", platform.system(), "x86_" + tag)
      check_dir(build_dir)
      check_dir(bin_dir)
      check_dir(lib_dir)
      os.chdir(bin_dir)
      ret = os.system("git init")
      # if(ret !=0): sys.exit(ret)
      ret = os.system("git add *")
      # if(ret !=0): sys.exit(ret)
      ret = os.system("git commit -m \"Commit\"")
      # if(ret !=0): sys.exit(ret)
      os.chdir(root)
      ret = os.system("py -3 patch_gen.py {} dist OverlayProc_x86_{}".format(bin_dir, tag))
      if(ret !=0): sys.exit(ret)
    
    if "QTDIR64" in os.environ:
      tag = get_qt_version(os.environ["QTDIR64"])
      build_dir = os.path.join(os.getcwd(), "build", platform.system(), "x64_" + tag)
      bin_dir = os.path.join(os.getcwd(), "bin", platform.system(), "x64_" + tag, "Release")
      lib_dir = os.path.join(os.getcwd(), "lib", platform.system(), "x64_" + tag)
      check_dir(build_dir)
      check_dir(bin_dir)
      check_dir(lib_dir)
      os.chdir(bin_dir)
      ret = os.system("git init")
      # if(ret !=0): sys.exit(ret)
      ret = os.system("git add *")
      # if(ret !=0): sys.exit(ret)
      ret = os.system("git commit -m \"Commit\"")
      # if(ret !=0): sys.exit(ret)
      os.chdir(root)
      ret = os.system("py -3 patch_gen.py {} dist OverlayProc_x86_{}".format(bin_dir, tag))
      if(ret !=0): sys.exit(ret)
