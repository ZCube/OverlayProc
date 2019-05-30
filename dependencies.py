import os
import platform

def check_dir(dir):
    if(not os.path.exists(dir)):
        os.makedirs(dir)

if "CMAKE_GENERATOR" in os.environ:
  generator = os.environ["CMAKE_GENERATOR"]
else:
  generator = "Visual Studio 14 2015"

def get_qt_version(dir):
  with open(os.path.join(dir, "mkspecs", "qconfig.pri"), 'rb') as f:
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
      build_dir = os.path.join(root, "build", platform.system(), "x86_" + tag)
      bin_dir = os.path.join(root, "bin", platform.system(), "x86_" + tag)
      check_dir(build_dir)
      check_dir(bin_dir)
      os.chdir(build_dir)
      os.chdir(bin_dir)
      if(os.path.exists(os.path.join(bin_dir,"Release"))):
        os.chdir(os.path.join(bin_dir,"Release"))
        os.system(os.path.join(root, "tools", "dependency.py"))
      os.chdir(root)
      
    if "QTDIR64" in os.environ:
      build_dir = os.path.join(root, "build", platform.system(), "x64_" + tag)
      bin_dir = os.path.join(root, "bin", platform.system(), "x64_" + tag)
      check_dir(build_dir)
      check_dir(bin_dir)
      os.chdir(build_dir)
      if(os.path.exists(os.path.join(bin_dir,"Release"))):
        os.chdir(os.path.join(bin_dir,"Release"))
        os.system(os.path.join(root, "tools", "dependency.py"))
      os.chdir(root)

    if "QTDIR32" in os.environ:
      build_dir = os.path.join(root, "build", platform.system(), "x86_" + tag)
      bin_dir = os.path.join(root, "bin", platform.system(), "x86_" + tag)
      check_dir(build_dir)
      check_dir(bin_dir)
      os.chdir(build_dir)
      os.chdir(bin_dir)
      print(bin_dir)
      if(os.path.exists(os.path.join(bin_dir,"Debug"))):
        os.chdir(os.path.join(bin_dir,"Debug"))
        os.system(os.path.join(root, "tools", "dependency.py"))
      os.chdir(root)

    if "QTDIR64" in os.environ:
      build_dir = os.path.join(root, "build", platform.system(), "x64_" + tag)
      bin_dir = os.path.join(root, "bin", platform.system(), "x64_" + tag)
      check_dir(build_dir)
      check_dir(bin_dir)
      os.chdir(build_dir)
      os.chdir(bin_dir)
      if(os.path.exists(os.path.join(bin_dir,"Debug"))):
        os.chdir(os.path.join(bin_dir,"Debug"))
        os.system(os.path.join(root, "tools", "dependency.py"))
      os.chdir(root)
