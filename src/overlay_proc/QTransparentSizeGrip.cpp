#include "qtransparentsizegrip.h"

QTransparentSizeGrip::QTransparentSizeGrip(QWidget *parent)
	: QSizeGrip(parent) {
}

void QTransparentSizeGrip::paintEvent(QPaintEvent *event)
{
	return;
}
