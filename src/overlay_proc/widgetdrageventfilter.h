#pragma once
#include <QWidget>

class WidgetDragEventFilter : public QObject
{
	Q_OBJECT
public:
	bool useDragFilter;
	bool useDragMove;
	QPoint mousePos;
	bool beginMove;
	bool useAppRegion;

	WidgetDragEventFilter(QWidget* parent = 0);
	virtual bool eventFilter(QObject* object, QEvent* event);
};
