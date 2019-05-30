#include "overlaymainwindow.h"
#include <QEvent>
#include <QMouseEvent>
#include <QApplication>
#include "qtransparentsizegrip.h"

OverlayMainWindow::OverlayMainWindow(QWidget *parent)
	: QMainWindow(parent) {
	view = nullptr;
	resizer = nullptr;
	resizer2 = nullptr;
	setMinimumSize(16, 16);
	filter = new WidgetDragEventFilter(this);
	appRegion = false;
}

OverlayMainWindow::~OverlayMainWindow()
{
	if (view)
	{
		view = nullptr;
	}
}

void OverlayMainWindow::InitSignalSlot()
{
}

void OverlayMainWindow::finishLoading(bool b)
{
}

void OverlayMainWindow::resizeEvent(QResizeEvent *event) {
	if (resizer != nullptr) {
		resizer->move(rect().bottomRight() - resizer->rect().bottomRight());
	}
	if (resizer2 != nullptr) {
		resizer2->move(0, 0);
	}
}

void OverlayMainWindow::moveEvent(QMoveEvent *e)
{
}

void OverlayMainWindow::mouseMoveEvent(QMouseEvent *e)
{
	//e->accept();
}

template <typename F>
static void postToThread(F && fun, QObject * obj = qApp) {
	QObject src;
	if (QApplication::instance()->thread() == QThread::currentThread())
	{
		fun();
	}
	else
	{
		QObject::connect(&src, &QObject::destroyed, obj, std::move(fun), Qt::QueuedConnection);
	}
}

void OverlayMainWindow::closeEvent(QCloseEvent *event)
{
	event->accept();
	if (view)
	{
		//this->view->setUrl(QUrl("about:blank"));
		//this->view->page()->deleteLater();
		//this->view->deleteLater();
		//this->view = nullptr;
		//disconnect(this->view->page());
		//disconnect(this->view);
		//this->view->page()->deleteLater();
		//this->view->close();
		//this->view = nullptr;

	}
	QMainWindow::closeEvent(event);
}

void OverlayMainWindow::setVisible(bool visible) {
	QMainWindow::setVisible(visible);

	if (resizer == nullptr) {
		resizer = new QTransparentSizeGrip(this);
		resizer->setWindowFlags(Qt::WindowStaysOnTopHint);
		resizer->setWindowOpacity(0.0);
		QSize qs = resizer->sizeHint();
		resizer->resize(qs);
		resizer->move(rect().bottomRight() - resizer->rect().bottomRight());
		resizer->setVisible(visible);
	}
	else {
		QSize qs = resizer->sizeHint();
		resizer->resize(qs);
		resizer->move(rect().bottomRight() - resizer->rect().bottomRight());
		resizer->setVisible(visible);
	}
	if (resizer2 == nullptr) {
		resizer2 = new QTransparentSizeGrip(this);
		resizer2->setWindowFlags(Qt::WindowStaysOnTopHint);
		resizer->setWindowOpacity(0.0);
		QSize qs = resizer2->sizeHint();
		resizer2->resize(qs);
		resizer2->move(rect().topLeft());
		resizer2->setVisible(visible);
	}
	else {
		QSize qs = resizer2->sizeHint();
		resizer2->resize(qs);
		resizer2->move(rect().topLeft());
		resizer2->setVisible(visible);
	}
}
