
IF(MSVC)
add_compile_options(/EHa) # enable C++ EH (w/ SEH exceptions)
ENDIF(MSVC)

ADD_DEFINITIONS(
  -DBOOST_ALL_NO_LIB
  -D_CRT_SECURE_NO_WARNINGS
  -D_WIN32_WINNT=0x0601
  -DWIN32_LEAN_AND_MEAN
  -DBOOST_COROUTINE_NO_DEPRECATION_WARNING
)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Widgets)
find_package(Qt5WinExtras)
find_package(Qt5LinguistTools)
find_package(Qt5Gui)
find_package(Qt5WebEngineWidgets)
find_package(Qt5WebSockets)

include_directories(${Qt5Gui_PRIVATE_INCLUDE_DIRS})

SET(overlay_proc_TS
  translate/overlay_proc_en.ts
  translate/overlay_proc_ko.ts
  translate/overlay_proc_ja.ts
)

ADD_DEFINITIONS(
	-DQT_QTSINGLEAPPLICATION_IMPORT
	-DQT_QTWINMIGRATE_IMPORT
)

SET(MAIN_SOURCES
    mainwindow.h
    mainwindow.cpp
    qonekeysequenceedit.h
    qonekeysequenceedit.cpp
    qtransparentsizegrip.h
    qtransparentsizegrip.cpp
    widgetdrageventfilter.h
    widgetdrageventfilter.cpp
    overlaywebenginepage.h
    overlaywebenginepage.cpp
    overlaymainwindow.h
    overlaymainwindow.cpp
    websocketserver.h
    websocketserver.cpp
    copydatawidget.h
    copydatawidget.cpp
	settings_impl.h
	settings_impl.cpp
    main.h
    main.cpp
	utility.h
	utility.cpp
    ../version.cpp
)

#QT5_CREATE_TRANSLATION(QMS 
#  main.cpp
#  mainwindow.ui
#  ${overlay_proc_TS}
#)
QT5_ADD_TRANSLATION(QMS ${overlay_proc_TS})

add_executable(overlay_proc
    mainwindow.ui
    ${QMS}
    ${MAIN_SOURCES}
    #configdialog.qrc
    resources.qrc
    Resource.rc
)

foreach(filename ${QMS})
  get_filename_component(name ${filename} NAME)
  add_custom_command(TARGET overlay_proc POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${filename} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/translations/${name}"
  )
  add_custom_command(TARGET overlay_proc POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${filename} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug/translations/${name}"
  )
endforeach()

INCLUDE_DIRECTORIES(
    ${CONAN_INCLUDE_DIRS}
)
LINK_DIRECTORIES(
    ${CONAN_LIB_DIRS}
)

target_link_libraries(overlay_proc
    #CONAN_PKG::jsoncpp
    ${CONAN_LIBS}
    settings
)

conan_target_link_libraries(overlay_proc)

target_link_libraries(overlay_proc 
  Qt5::Core
  Qt5::Widgets
  Qt5::WebEngineWidgets
  Qt5::WebSockets)


add_executable(change_renderer
    changerenderer.ui
    changerenderer.h
    changerenderer.cpp
    Resource.rc
)

target_link_libraries(change_renderer
    #CONAN_PKG::jsoncpp
    ${CONAN_LIBS}
)

conan_target_link_libraries(change_renderer)

target_link_libraries(change_renderer 
  Qt5::Core
  Qt5::Widgets)

IF(WIN32) # Check if we are on Windows
  if(MSVC) # Check if we are using the Visual Studio compiler
    set_target_properties(overlay_proc PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
    target_link_libraries(overlay_proc ${Qt5Core_QTMAIN_LIBRARIES})
    set_target_properties(change_renderer PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
    target_link_libraries(change_renderer ${Qt5Core_QTMAIN_LIBRARIES})
  elseif(CMAKE_COMPILER_IS_GNUCXX)
    # SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows") # Not tested
  else()
    message(SEND_ERROR "You are using an unsupported Windows compiler! (Not MSVC or GCC)")
  endif()
elseif(UNIX)
  # Nothing special required
else()
  message(SEND_ERROR "You are on an unsupported platform! (Not Win32 or Unix)")
ENDIF()
