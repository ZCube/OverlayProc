import os
import platform
import sys

def check_dir(dir):
    if(not os.path.exists(dir)):
        os.makedirs(dir)

if "CMAKE_GENERATOR" in os.environ:
  generator = os.environ["CMAKE_GENERATOR"]
else:
  generator = "Visual Studio 14 2015"

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
    os.chdir("src")
    ret = os.system("rev.py")
    os.chdir(root)
    
    if "QTDIR32" in os.environ:
      tag = get_qt_version(os.environ["QTDIR32"])
      build_dir = os.path.join(os.getcwd(), "build", platform.system(), "x86_" + tag)
      bin_dir = os.path.join(os.getcwd(), "bin", platform.system(), "x86_" + tag)
      lib_dir = os.path.join(os.getcwd(), "lib", platform.system(), "x86_" + tag)
      check_dir(build_dir)
      check_dir(bin_dir)
      check_dir(lib_dir)
      os.chdir(build_dir)
      ret = os.system("cmake --build . --config Release")
      if(ret !=0): sys.exit(ret)
      os.chdir(bin_dir)
      os.chdir(os.path.join(bin_dir,"Release"))
      ret = os.system(os.path.join(root, "tools", "dependency.py"))
      if(ret !=0): sys.exit(ret)
      os.chdir(root)
    
    if "QTDIR64" in os.environ:
      tag = get_qt_version(os.environ["QTDIR64"])
      build_dir = os.path.join(os.getcwd(), "build", platform.system(), "x64_" + tag)
      bin_dir = os.path.join(os.getcwd(), "bin", platform.system(), "x64_" + tag)
      lib_dir = os.path.join(os.getcwd(), "lib", platform.system(), "x64_" + tag)
      check_dir(build_dir)
      check_dir(bin_dir)
      check_dir(lib_dir)
      os.chdir(build_dir)
      ret = os.system("cmake --build . --config Release")
      if(ret !=0): sys.exit(ret)
      os.chdir(bin_dir)
      os.chdir(os.path.join(bin_dir,"Release"))
      ret = os.system(os.path.join(root, "tools", "dependency.py"))
      if(ret !=0): sys.exit(ret)
      os.chdir(root)

    # build_dir = os.path.join(os.getcwd(), "build", platform.system(), "x86")
    # bin_dir = os.path.join(os.getcwd(), "bin", platform.system(), "x86")
    # check_dir(build_dir)
    # check_dir(bin_dir)
    # os.chdir(build_dir)
    # ret = os.system("cmake --build . --config Debug")
    # if(ret !=0): sys.exit(ret)
    # os.chdir(bin_dir)
    # os.chdir(os.path.join(bin_dir,"Debug"))
    # ret = os.system(os.path.join(root, "tools", "dependency.py"))
    # if(ret !=0): sys.exit(ret)
    # os.chdir(root)
    