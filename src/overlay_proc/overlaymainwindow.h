#pragma once

#include <QWidget>
#include <QMainWindow>
#include <boost/uuid/uuid.hpp>
#include <QSizeGrip>
#include <widgetdrageventfilter.h>
#include <QWebEngineView>
#include <QWidget>
#ifdef _WIN32
#include <Windows.h>
#endif

class OverlayMainWindow : public QMainWindow
{
	Q_OBJECT
public:
	boost::uuids::uuid id;
	QSizeGrip* resizer;
	QSizeGrip* resizer2;
	QWebEngineView* view;
	WidgetDragEventFilter* filter;
	bool appRegion;
	QString lastURL = "";
	OverlayMainWindow(QWidget* parent = nullptr);
	virtual ~OverlayMainWindow();
	void InitSignalSlot();
	virtual void resizeEvent(QResizeEvent * event);
	virtual void moveEvent(QMoveEvent * event);
	void mouseMoveEvent(QMouseEvent *e);
	virtual void closeEvent(QCloseEvent * event);
	virtual void setVisible(bool visible);
	public slots:
	void finishLoading(bool b);
};
