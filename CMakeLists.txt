cmake_minimum_required(VERSION 3.1)

project(OverlayProject C CXX)
#suppress warning message
cmake_policy(SET CMP0020 OLD)
cmake_policy(SET CMP0043 OLD)

set (CMAKE_CXX_STANDARD 14)

# Download automatically, you can also just copy the conan.cmake file
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
   message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
   file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.14/conan.cmake"
                 "${CMAKE_BINARY_DIR}/conan.cmake")
endif()

include (CheckTypeSize)
CHECK_TYPE_SIZE("void*"  CMAKE_SIZEOF_VOID_P)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  if(DEFINED ENV{QTDIR32})
    SET(QTDIR_ROOT "$ENV{QTDIR32}")
  elseif(DEFINED ENV{QTDIR})
    SET(QTDIR_ROOT "$ENV{QTDIR}")
  endif()
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
  if(DEFINED ENV{QTDIR64})
    SET(QTDIR_ROOT "$ENV{QTDIR64}")
  elseif(DEFINED ENV{QTDIR})
    SET(QTDIR_ROOT "$ENV{QTDIR}")
  endif()
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  configure_file(conanfile.py "${CMAKE_CURRENT_SOURCE_DIR}/_conanfile86.py" @ONLY)
  conan_cmake_run(CONANFILE "_conanfile86.py"
                BASIC_SETUP
                SETTINGS arch=x86
                BUILD missing)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
  configure_file(conanfile.py "${CMAKE_CURRENT_SOURCE_DIR}/_conanfile64.py" @ONLY)
  conan_cmake_run(CONANFILE "_conanfile64.py"
                BASIC_SETUP
                SETTINGS arch=x86_64
                BUILD missing)
endif()

conan_define_targets()

ADD_DEFINITIONS(
  ${CONAN_DEFINES_BOOST}
  -D_CRT_SECURE_NO_WARNINGS
  -D_WIN32_WINNT=0x0601
  -DWIN32_LEAN_AND_MEAN
  -DUNICODE
  -D_UNICODE
)

if(NOT DEFINED QTDIR_ROOT)
  # mac  export QTDIR=/usr/local/Cellar/qt5/5.4.1/
  file(GLOB QTDIR_ROOT /usr/local/Cellar/qt5/*/)
endif(NOT DEFINED QTDIR_ROOT)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${QTDIR_ROOT})
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

ADD_SUBDIRECTORY(src)

IF(MSVC)
  OPTION(USE_STATIC_RUNTIME_LIBRARY "Use Static Runtime Library" OFF)
  OPTION(USE_LINK_TIME_OPTIMIZATION "Use Link Time Optimization" ON)

  foreach (flag_var
           CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
           CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
           CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
           CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)

    set(_PARAM "")
    IF(USE_STATIC_RUNTIME_LIBRARY)
      string(FIND "${${flag_var}}" "/MT" _TESTVAR)
      if(${_TESTVAR} EQUAL "-1")
        string(FIND "${${flag_var}}" "/MD" _TESTVAR)
        if(${_TESTVAR} EQUAL "-1")
          set(_PARAM "${_PARAM} /MT ")
        else()
          string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        ENDIF()
      ENDIF()
    ENDIF()
    IF(USE_LINK_TIME_OPTIMIZATION)
      set(_PARAM "${_PARAM} /GL ")
    ENDIF()
    
    # multi processor
    string(FIND "${${flag_var}}" "/MP" _TESTVAR)
    if(${_TESTVAR} EQUAL "-1")
      set(_PARAM "${_PARAM} /MP")
    endif()
    if(_PARAM)
      set(${flag_var} "${${flag_var}} ${_PARAM}" CACHE STRING "" FORCE)
    ENDIF()
    
  endforeach()

  IF(USE_LINK_TIME_OPTIMIZATION)
      message(STATUS "- MSVC: Enabled multiprocess build option")
      foreach (flag_var
               CMAKE_EXE_LINKER_FLAGS CMAKE_EXE_LINKER_FLAGS_DEBUG CMAKE_EXE_LINKER_FLAGS_RELEASE
               CMAKE_EXE_LINKER_FLAGS_MINSIZEREL CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO
               CMAKE_SHARED_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS_DEBUG CMAKE_SHARED_LINKER_FLAGS_RELEASE
               CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO
               CMAKE_MODULE_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS_DEBUG CMAKE_MODULE_LINKER_FLAGS_RELEASE
               CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO)
        string(FIND "${${flag_var}}" "/LTCG" _TESTVAR)
        if(${_TESTVAR} EQUAL "-1")
          set(${flag_var} "${${flag_var}} /LTCG" CACHE STRING "" FORCE)
        endif()
      endforeach()
      foreach (flag_var
               CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
               CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
               CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
               CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
        string(REPLACE "${${flag_var}}" "${${flag_var}} /GL" ${flag_var} "${${flag_var}}")
      endforeach()
  ENDIF(USE_LINK_TIME_OPTIMIZATION)
ENDIF()
 
IF(MSVC)
  if(CMAKE_SIZEOF_VOID_P MATCHES 4)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
    message(STATUS "- MSVC: Enabled large address awareness")
  endif()
ELSE(MSVC)
ENDIF(MSVC)
