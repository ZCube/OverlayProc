INCLUDE_DIRECTORIES(
	../external/jsoncpp/include
	../external/boost
	../external/websocketpp/
	../external/qt-solutions/src/qtsingleapplication/src
	../external/qt-solutions/src/qtwinmigrate/src
	../external
	.
)

ADD_SUBDIRECTORY(overlay_proc)
ADD_SUBDIRECTORY(settings)
