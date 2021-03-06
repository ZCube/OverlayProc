import os
import platform
import sys

def check_dir(dir):
    if(not os.path.exists(dir)):
        os.makedirs(dir)

if "CMAKE_GENERATOR" in os.environ:
  generator = os.environ["CMAKE_GENERATOR"]
else:
  generator = "Visual Studio 15 2017"

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
      bin_dir = os.path.join(os.getcwd(), "bin", platform.system(), "x86_" + tag)
      lib_dir = os.path.join(os.getcwd(), "lib", platform.system(), "x86_" + tag)
      check_dir(build_dir)
      check_dir(bin_dir)
      check_dir(lib_dir)
      os.chdir(build_dir)
      os.environ['CMAKE_GENERATOR_PLATFORM'] = "win32"
      ret = os.system("cmake ../../.. -G \"{}\" ".format(generator) +
          "\"-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=" + lib_dir + "\" "
          "\"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=" + bin_dir + "\" "
          "\"-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=" + bin_dir + "\" "
      )
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
      os.environ['CMAKE_GENERATOR_PLATFORM'] = "x64"
      if not generator.startswith("Visual Studio 16 2019"):
        generator = "{} Win64".format(generator)
      ret = os.system("cmake ../../.. -G \"{}\" ".format(generator) +
          "\"-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY:PATH=" + lib_dir + "\" "
          "\"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY:PATH=" + bin_dir + "\" "
          "\"-DCMAKE_RUNTIME_OUTPUT_DIRECTORY:PATH=" + bin_dir + "\" "
      )
      if(ret !=0): sys.exit(ret)
      os.chdir(root)
