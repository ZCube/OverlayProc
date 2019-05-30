#include <QApplication>
#include <QtWebEngineWidgets/QtWebEngineWidgets>
#include <iostream>
#include <QMainWindow>
#include <QWidget>
#include <json/json.h>
#include <settings/settings.h>
#include <QCloseEvent>

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QByteArray>

#include "QTransparentSizeGrip.h"
#include "widgetdrageventfilter.h"
#include "overlaymainwindow.h"
#include "utility.h"

#ifdef _WIN32
#include <Windows.h>
#endif
