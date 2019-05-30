import functools
import operator
import subprocess
import os
import re
import glob
import shutil
import platform
import tempfile
import sys

dumpbin = shutil.which("dumpbin")
if dumpbin == None:
  dumpbin = shutil.which("dumpbin", path = ";".join(glob.glob("C:/Program Files (x86)/Microsoft Visual Studio/*/*/VC/Tools/MSVC/*/bin/*/*/") +["C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/bin/"]))

last_version=None
def files_copy_executable(files, dest, debug):
    for f in files:
        filename = os.path.splitext(os.path.basename(f))[0]
        if debug and filename.endswith("d") :
            copy_newer(f, os.path.join(dest, os.path.basename(f)))
        elif not debug and not filename.endswith("d") :
            copy_newer(f, os.path.join(dest, os.path.basename(f)))

def files_copy(files, dest):
    for f in files:
        filename = os.path.splitext(os.path.basename(f))[0]
        copy_newer(f, os.path.join(dest, os.path.basename(f)))

def copy_newer(src, dest):
  print(src, dest)
  if os.path.isdir(src):
    if os.path.exists(dest):
      for f in glob.glob(os.path.join(src, "*")):
        copy_newer(f, os.path.join(dest, os.path.basename(f)))
    else:
      os.makedirs(dest)
      for f in glob.glob(os.path.join(src, "*")):
        copy_newer(f, os.path.join(dest, os.path.basename(f)))
  else:
    if os.path.exists(dest):
      if os.stat(src).st_mtime - os.stat(dest).st_mtime > 1:
        shutil.copy2(src, dest)
    else:
      shutil.copy2(src, dest)
    

def fwrite(f, str):
  if sys.version_info > (3, 0):
    os.write(f, bytes(str, "UTF-8"))
  else:
    os.write(f, str)
def getLastVisualStudioVersion():
  vsCommonToolsPattern = re.compile(r"VS(\d+)COMNTOOLS", re.UNICODE)

  keys = list(os.environ.keys())
  keys.sort()

  last_version = 0
  for item in keys:
    m = re.match(vsCommonToolsPattern, item)
    if m:
      if not m.group(1) == None:
        last_version = max(last_version, int(m.group(1)))
    
  # if version > 0:
    # print ("VS%dCOMNTOOLS" % version)
  return last_version / 10
def getVisualStudioCommonDir(v):
  verString = ("VS%dCOMNTOOLS" % (v*10))
  if verString in os.environ:
    return os.environ[verString]
  else:
    if last_version !=None and last_version == 15 and os.path.exists(os.path.join(os.environ["ProgramFiles(x86)"], "Microsoft Visual Studio", "2017")):
      msvc_common_dir = os.path.exists(os.path.join(os.environ["ProgramFiles(x86)"], "Microsoft Visual Studio", "2017"))
  return ""
def getCurrentVisualStudioVersion():
  found = False
  last_version = getLastVisualStudioVersion()
  if "VSINSTALLDIR" in os.environ:
    for v in range(int(last_version+1)):
      verString = ("VS%dCOMNTOOLS" % (v*10))
      if verString in os.environ and os.environ[verString].lower().startswith(os.environ["VSINSTALLDIR"].lower()):
        found = True
        # msvc_common_dir = os.environ[verString]
        return v
  else:
    for v in range(int(last_version+1)):
      verString = ("VS%dCOMNTOOLS" % (v*10))
      if verString in os.environ:
        found = True
        return v
    if os.path.exists(os.path.join(os.environ["ProgramFiles(x86)"], "Microsoft Visual Studio", "2017")):
      return 15
  return 0

def call_by_script(cmd, architecture):
  if platform.system() == 'Windows' :
    # os.system은 실행 할 수 있는 명령  길이가 제한됨 따라서 스크립트를 만들어서 실행
    f, fname = tempfile.mkstemp(suffix='.bat')

    version = getCurrentVisualStudioVersion()
    if version != 0:
      envString = ("VS%dCOMNTOOLS" % (version*10))
      if architecture != None:
        if architecture.find("x64") >= 0:
          architecture = "amd64"
        else:
          architecture = "x86"
      else:
        architecture = "x86"
        
      fwrite(f, ("call \"" + getVisualStudioCommonDir(version)+"\\..\\..\\VC\\vcvarsall.bat\" {0}\n".format(architecture)))
        
    fwrite(f, "@")
    fwrite(f, cmd)
    os.close(f)
    p = subprocess.Popen([fname], stdout=subprocess.PIPE, 
                                        stderr=subprocess.PIPE)
    out, err = p.communicate()
    os.remove(fname)
    return (str(out, 'utf-8', 'ignore'), str(err))
  else :
    # os.system은 실행 할 수 있는 명령  길이가 제한됨 따라서 스크립트를 만들어서 실행
    f, fname = tempfile.mkstemp(suffix='.sh')
    os.close(f)
    os.open(fname, os.O_WRONLY | os.O_CREAT)
    fwrite(f, "#!/bin/bash\n")
    fwrite(f, cmd)
    fwrite(f, "\nexit $?\n")
    os.close(f)
    os.chmod(fname, stat.S_IRWXU | stat.S_IXUSR | stat.S_IRUSR | stat.S_IXOTH | stat.S_IROTH)
    p = subprocess.Popen([fname], stdout=subprocess.PIPE, 
                                        stderr=subprocess.PIPE)
    out, err = p.communicate()
    os.remove(fname)
    return (str(out), str(err))

excludes = [
    "ADVAPI32.dll",
    "BluetoothApis.dll",
    "CFGMGR32.dll",
    "COMCTL32.dll",
    "COMDLG32.dll",
    "CRYPT32.dll",
    "DNSAPI.dll",
    "GDI32.dll",
    "IMM32.dll",
    "IPHLPAPI.DLL",
    "KERNEL32.dll",
    "MF.dll",
    "MFPlat.DLL",
    "MFReadWrite.dll",
    "MPR.dll",
    "OLEAUT32.dll",
    "POWRPROF.dll",
    "PSAPI.DLL",
    "RPCRT4.dll",
    "SETUPAPI.dll",
    "SHELL32.dll",
    "SHLWAPI.dll",
    "Secur32.dll",
    "USER32.dll", 
    "USERENV.dll",
    "USP10.dll",
    "VERSION.dll",
    "WINHTTP.dll",
    "WINMM.dll",
    "WS2_32.dll",
    "d3d11.dll",
    "d3d9.dll",
    "dbghelp.dll",
    "dhcpcsvc.DLL",
    "dwmapi.dll",
    "dxva2.dll",
    "ole32.dll",
    "urlmon.dll",
    "OLEACC.dll",
    "WTSAPI32.dll",
    "UxTheme.dll",
    "DWrite.dll",
    "Netapi32.dll",
    "MSVCRT.dll",
    "dxgi.dll",
    "imagehlp.dll"
  ]
  
import ctypes
k32 = ctypes.windll.kernel32
wow64 = ctypes.c_long( 0 )
k32.Wow64DisableWow64FsRedirection( ctypes.byref(wow64) )

root = (os.path.abspath(os.path.dirname(os.path.realpath(__file__))) + "/").replace("/", os.sep)
qtdir = None
qtdir64 = None
if 'QTDIR32' in os.environ:
  qtdir = os.environ['QTDIR32']
if qtdir == None or len(qtdir) == 0:
  if 'QTDIR' in os.environ:
    qtdir = os.environ['QTDIR']
  else:
    qtdir = None
if 'QTDIR64' in os.environ:
  qtdir64 = os.environ['QTDIR64']

vcdir = "C:/Program Files (x86)/Microsoft Visual Studio 14.0/VC/"

dirs = [
    vcdir+"redist/debug_nonredist/x86/Microsoft.*.DebugCRT",
    vcdir+"redist/debug_nonredist/x86/Microsoft.*.DebugCXXAMP",
    vcdir+"redist/debug_nonredist/x86/Microsoft.*.DebugMFC",
    vcdir+"redist/debug_nonredist/x86/Microsoft.*.DebugOPENMP",
    root+"prebuilt/cef/x86/Debug",
    "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Developer Runtime/x86",
]
dlls_x86_debug = [functools.reduce(operator.add,([glob.glob(y)])) for y in [os.path.join(x, "*.dll") for x in dirs]]
dlls_x86_debug = functools.reduce(operator.add,(dlls_x86_debug))
if qtdir != None:
  dlls_x86_debug = dlls_x86_debug + glob.glob(os.path.join(qtdir, "bin", "*d.dll"))
dirs = [
    vcdir+"redist/debug_nonredist/x64/Microsoft.*.DebugCXXAMP",
    vcdir+"redist/debug_nonredist/x64/Microsoft.*.DebugMFC",
    vcdir+"redist/debug_nonredist/x64/Microsoft.*.DebugOpenMP",
    vcdir+"redist/debug_nonredist/x64/Microsoft.*.DebugCRT",
    root+"prebuilt/cef/x64/Debug",
    "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Developer Runtime/x64",
]
dlls_x64_debug = [functools.reduce(operator.add,([glob.glob(y)])) for y in [os.path.join(x, "*.dll") for x in dirs]]
dlls_x64_debug = functools.reduce(operator.add,(dlls_x64_debug))
if qtdir64 != None:
  dlls_x64_debug = dlls_x64_debug + glob.glob(os.path.join(qtdir64, "bin", "*d.dll"))
dirs = [
    vcdir+"redist/x86/Microsoft.*.MFC",
    vcdir+"redist/x86/Microsoft.*.MFCLOC",
    vcdir+"redist/x86/Microsoft.*.OPENMP",
    vcdir+"redist/x86/Microsoft.*.CRT",
    vcdir+"redist/x86/Microsoft.*.CXXAMP",
    root+"prebuilt/cef/x86/Release",
    "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Developer Runtime/x86",
]
dlls_x86_release = [functools.reduce(operator.add,([glob.glob(y)]))  for y in [os.path.join(x, "*.dll") for x in dirs]]
dlls_x86_release = functools.reduce(operator.add,(dlls_x86_release))
if qtdir != None:
  dlls_x86_release = dlls_x86_release + glob.glob(os.path.join(qtdir, "bin", "*.dll"))
dirs = [
    vcdir+"redist/x64/Microsoft.*.CXXAMP",
    vcdir+"redist/x64/Microsoft.*.MFC",
    vcdir+"redist/x64/Microsoft.*.MFCLOC",
    vcdir+"redist/x64/Microsoft.*.OpenMP",
    vcdir+"redist/x64/Microsoft.*.CRT",
    root+"prebuilt/cef/x64/Release",
    "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Developer Runtime/x64",
]
dlls_x64_release = [functools.reduce(operator.add,([glob.glob(y)])) for y in [os.path.join(x, "*.dll") for x in dirs]]
dlls_x64_release = functools.reduce(operator.add,(dlls_x64_release))
if qtdir64 != None:
  dlls_x64_release = dlls_x64_release + glob.glob(os.path.join(qtdir64, "bin", "*.dll"))

def check(f, excludes):
    for e in excludes:
        r = re.findall(e, f, re.I)
        if (len(r)>0):
            return True
    return False

x64 = True if "x64" in os.getcwd().lower() else False
debug = True if "debug" in os.path.basename(os.getcwd()).lower() else False
qtdir = qtdir64 if "x64" in os.getcwd().lower() else qtdir

dirs = [
    "C:/Program Files (x86)/Microsoft Visual Studio/*/BuildTools/VC/Redist/MSVC/*/x86/Microsoft.VC*.CRT",
    "C:/Program Files (x86)/Windows Kits/10/Redist/ucrt/DLLs/x86",
]
dlls_x86_ucrt = [functools.reduce(operator.add,([glob.glob(y)])) for y in [os.path.join(x, "*.dll") for x in dirs]]
dlls_x86_ucrt = functools.reduce(operator.add,(dlls_x86_ucrt))
dirs = [
    "C:/Program Files (x86)/Microsoft Visual Studio/*/BuildTools/VC/Redist/MSVC/*/x64/Microsoft.VC*.CRT",
    "C:/Program Files (x86)/Windows Kits/10/Redist/ucrt/DLLs/x64",
]    
dlls_x64_ucrt = [functools.reduce(operator.add,([glob.glob(y)])) for y in [os.path.join(x, "*.dll") for x in dirs]]
dlls_x64_ucrt = functools.reduce(operator.add,(dlls_x64_ucrt))

cnt = 1
while cnt > 0:
    cnt = 0
    needed = []
    out, err = call_by_script("\"{}\" /dependents /headers *.dll *.exe".format(dumpbin), "")
    out = str(out).replace("\r", "")
    
    # x64 = "machine (x64)" in out
    if(not x64 and debug):
        dlls = dlls_x86_debug
    if(x64 and debug):
        dlls = dlls_x64_debug
    if(not x64 and not debug):
        dlls = dlls_x86_release
    if(x64 and not debug):
        dlls = dlls_x64_release


    ls = [x.strip() for x in out.split("\n")]
    ls = [x.strip() for x in ls if len(re.findall(".dll", x, re.I)) > 0]
    ls = [x.strip() for x in ls if not len(re.findall("dll.pdb", x, re.I)) > 0]
    ls = [x.strip() for x in ls if not len(re.findall("DLL characteristics", x, re.I)) > 0]
    ls = [x.strip() for x in ls if not len(re.findall("Dump of file", x, re.I)) > 0]
    ls = [x.strip() for x in ls if not len(re.findall("File Type:", x, re.I)) > 0]
    ls = [x.strip() for x in ls if not len(re.findall("Format:", x, re.I)) > 0]
    ls = [x.strip() for x in ls if not len(re.findall("entry point", x, re.I)) > 0]
    ls = (list(set(ls)))
    ls.sort()

    for i in range(len(ls)):
        if(len(ls[i]) > 0):
            if not os.path.exists(ls[i]) and not check(ls[i], excludes):
                if ls[i].lower().startswith("ucrtbase") or ls[i].lower().startswith("api-ms") :
                    if not x64 :
                      files_copy(dlls_x86_ucrt, ".")
                    if x64 :
                      files_copy(dlls_x64_ucrt, ".")
                try:
                    j = [os.path.basename(x).lower() for x in dlls].index(ls[i].lower())
                    copy_newer(dlls[j], ls[i].lower())
                    cnt = cnt + 1
                except ValueError:
                    print("needed " + ls[i])
                    needed.append(ls[i])

root = (os.path.abspath(os.path.dirname(os.path.realpath(__file__)) + "/../") + "/").replace("/", os.sep)

files = glob.glob(os.path.join(qtdir, "bin", "QtWebEngine*.exe")) \
      + glob.glob(os.path.join(qtdir, "bin", "libEGL*.dll")) \
      + glob.glob(os.path.join(qtdir, "bin", "libGLESv2*.dll"))
files_copy_executable(files, ".", debug)

files = []
# if x64:
  # if debug:
    # files += glob.glob(os.path.join(root + "prebuilt/cef/x64", "Debug", "*.bin"))
    # files += glob.glob(os.path.join(root + "prebuilt/cef/x64", "Debug", "*.dll"))
    # files += glob.glob(os.path.join(root + "prebuilt/cef/x64", "Debug", "*.exe"))
    # files += glob.glob(os.path.join(root + "prebuilt/cef/x64", "Resources", "*"))
  # else:
    # files += glob.glob(os.path.join(root + "prebuilt/cef/x64", "Release", "*.bin"))
    # files += glob.glob(os.path.join(root + "prebuilt/cef/x64", "Release", "*.dll"))
    # files += glob.glob(os.path.join(root + "prebuilt/cef/x64", "Release", "*.exe"))
    # files += glob.glob(os.path.join(root + "prebuilt/cef/x64", "Resources", "*"))
# else:      
  # if debug:
    # files += glob.glob(os.path.join(root + "prebuilt/cef/x86", "Debug", "*.bin"))
    # files += glob.glob(os.path.join(root + "prebuilt/cef/x86", "Debug", "*.dll"))
    # files += glob.glob(os.path.join(root + "prebuilt/cef/x86", "Debug", "*.exe"))
    # files += glob.glob(os.path.join(root + "prebuilt/cef/x86", "Resources", "*"))
  # else:
    # files += glob.glob(os.path.join(root + "prebuilt/cef/x86", "Release", "*.bin"))
    # files += glob.glob(os.path.join(root + "prebuilt/cef/x86", "Release", "*.dll"))
    # files += glob.glob(os.path.join(root + "prebuilt/cef/x86", "Release", "*.exe"))
    # files += glob.glob(os.path.join(root + "prebuilt/cef/x86", "Resources", "*"))

if x64:
    files += glob.glob(os.path.join(root + "prebuilt/openssl/x64", "bin", "*.dll"))
else:      
    files += glob.glob(os.path.join(root + "prebuilt/openssl/x86", "bin", "*.dll"))
files_copy(files, ".")

if not os.path.exists("platforms"):
    os.makedirs("platforms")
files = glob.glob(os.path.join(qtdir, "plugins", "platforms", "*.dll"))
files_copy_executable(files, "platforms", debug)

if not os.path.exists("styles"):
    os.makedirs("styles")
files = glob.glob(os.path.join(qtdir, "plugins", "styles", "*.dll"))
files_copy_executable(files, "styles", debug)

files = glob.glob(os.path.join(qtdir, "bin", "d3dcompiler_47.dll")) \
      + glob.glob(os.path.join(qtdir, "bin", "opengl32sw.dll"))
files_copy(files, ".")
    
if not os.path.exists("translations/qtwebengine_locales"):
    os.makedirs("translations/qtwebengine_locales")
files = glob.glob(os.path.join(qtdir, "translations/qtwebengine_locales", "*"))
files_copy(files, "translations/qtwebengine_locales")

if not os.path.exists("resources"):
    os.makedirs("resources")
files = glob.glob(os.path.join(qtdir, "resources", "*"))
files_copy(files, "resources")

copy_newer(os.path.join(root, "qt.conf"), os.path.join(".", "qt.conf"))

k32.Wow64RevertWow64FsRedirection( wow64 )
