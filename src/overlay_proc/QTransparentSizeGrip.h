#pragma once
#include <QtGui>
#include <QSizeGrip>

class QTransparentSizeGrip : public QSizeGrip
{
	Q_OBJECT
public:
	QTransparentSizeGrip(QWidget* parent = nullptr);
	virtual void paintEvent(QPaintEvent * event);
};
