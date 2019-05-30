#include <QtGui>
#include <QApplication>
#include "widgetdrageventfilter.h"
#include "overlaymainwindow.h"

#include <boost/lexical_cast.hpp>
#include "mainwindow.h"

extern MainWindow* manager;

WidgetDragEventFilter::WidgetDragEventFilter(QWidget *parent)
	: QObject(parent)
{
	useDragFilter = true;
	useDragMove = true;
	beginMove = false;
	useAppRegion = true;
}

bool WidgetDragEventFilter::eventFilter(QObject *object, QEvent *event)
{
	if (useDragFilter)
	{
		switch (event->type())
		{
		case QEvent::MouseMove:
		{
			if (beginMove) {
				QMouseEvent* qe = (QMouseEvent*)event;
				if (qe->buttons() & Qt::LeftButton)
				{
					QPoint diff = qe->pos() - mousePos;
					QPoint newpos = ((QWidget*)this->parent())->pos() + diff;

					if (useDragMove)
					{
						((QWidget*)this->parent())->move(newpos);
					}
				}
			}
			//if(mainwindow)
			if(!manager->DragModifiers())
			{
				beginMove = false;
			}
		}
		break;
		case QEvent::MouseButtonPress:
		{
			QMouseEvent* qe = (QMouseEvent*)event;
			if (qe->buttons() & Qt::LeftButton)
			{
				if (manager->DragModifiers())
				{
					if (useAppRegion)
					{
						OverlayMainWindow* wm = (OverlayMainWindow*)parent();
						bool appRegion = false;;
						mousePos = qe->pos();
						if (wm->view && wm->view->page())
						{
							wm->view->page()->runJavaScript("window.overlayAppRegionDrag", [this, wm, &appRegion](const QVariant &v) {
								appRegion = v.toBool();
								printf("%d\r\n", appRegion);
								if (appRegion)
								{
									beginMove = true;
								}
							});
						}
					}
					else
					{
						beginMove = true;
						mousePos = qe->pos();
					}
				}
			}
		}
		qDebug() << "MouseButtonPress" << " " << ((QMouseEvent*)event)->button() << " " << ((QMouseEvent*)event)->buttons() << " " << object;
		break;
		case QEvent::MouseButtonRelease:
			qDebug() << "MouseButtonRelease" << " " << ((QMouseEvent*)event)->button() << " " << ((QMouseEvent*)event)->buttons() << " " << object;
			beginMove = false;
			break;
		case QEvent::HoverEnter:
			qDebug() << "HoverEnter";
			break;
		case QEvent::HoverLeave:
			qDebug() << "HoverLeave";
			break;
		case QEvent::HoverMove:
			qDebug() << "HoverMove";
			break;
		}
	}
	if (true)
	{
		switch (event->type())
		{
		case QEvent::MouseMove:
		{
		}
		break;
		case QEvent::MouseButtonPress:
		{
		}
		break;
		case QEvent::MouseButtonRelease:
		{
			QMouseEvent* qe = (QMouseEvent*)event;
			if (qe->buttons() & Qt::BackButton)
			{
				OverlayMainWindow* m = (OverlayMainWindow*) this->parent();
				m->view->scroll(0, 0);
				////m->view->pageAction(QWebEnginePage::WebAction::)
				//this->parent()
			}
			if (qe->buttons() & Qt::ForwardButton)
			{

			}
		}
		break;
		case QEvent::HoverEnter:
			break;
		case QEvent::HoverLeave:
			break;
		case QEvent::HoverMove:
			break;
		}
	}
	return false;
	return QObject::eventFilter(object, event);
}
