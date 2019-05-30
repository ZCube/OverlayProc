from conans import ConanFile, CMake
import os

class DependencyConan(ConanFile):
  settings = "os", "compiler", "build_type", "arch"
  requires = "boost/1.70.0@conan/stable", "OpenSSL/latest_1.0.2x@conan/stable" ,"jsoncpp/1.8.4@theirix/stable" # comma-separated list of requirements
  generators = "cmake"
  default_options = "boost:shared=False", "OpenSSL:shared=False", "jsoncpp:shared=False", "boost:without_stacktrace=True"

  def imports(self):
    if self.settings.build_type == "Debug":
      self.copy("*.dll", dst=os.path.join("@CMAKE_RUNTIME_OUTPUT_DIRECTORY@", "Debug"), src="bin")
      self.copy("*.dylib*", dst=os.path.join("@CMAKE_RUNTIME_OUTPUT_DIRECTORY@", "Debug"), src="lib")
    elif self.settings.build_type == "Release":
      self.copy("*.dll", dst=os.path.join("@CMAKE_RUNTIME_OUTPUT_DIRECTORY@", "Release"), src="bin")
      self.copy("*.dylib*", dst=os.path.join("@CMAKE_RUNTIME_OUTPUT_DIRECTORY@", "Release"), src="lib")
