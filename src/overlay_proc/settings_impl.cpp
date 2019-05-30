#include "main.h"
#include "overlaymainwindow.h"
#include "overlaywebenginepage.h"
#include "settings_impl.h"
#include "utility.h"
#include <boost/lexical_cast.hpp>
#include "mainwindow.h"
#include <QGridLayout>
#include "../version.h"

QOverlaySettingsServer settingServer;
extern MainWindow* manager;
extern QString globalBackground;

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

void OverlaySettingsManager::UpdatePositions()
{
	OverlayMainWindow* w = nullptr;
	{
		std::lock_guard<std::recursive_mutex> l(settingServer.m);

		std::map<boost::uuids::uuid, OverlayMainWindow*>::iterator i;
		if ((i = settingServer.widgets.find(tag)) != settingServer.widgets.end())
		{
			w = i->second;
		}
	}
	if (w)
	{
		auto pos = w->pos();
		auto size = w->size();
		this->settings.x = pos.x();
		this->settings.y = pos.y();
		this->settings.width = size.width();
		this->settings.height = size.height();
	}
}

void OverlaySettingsManager::UpdateNextPositions()
{
	OverlayMainWindow* w = nullptr;
	{
		std::lock_guard<std::recursive_mutex> l(settingServer.m);

		std::map<boost::uuids::uuid, OverlayMainWindow*>::iterator i;
		if ((i = settingServer.widgets.find(tag)) != settingServer.widgets.end())
		{
			w = i->second;
		}
	}
	if (w)
	{
		auto pos = w->pos();
		auto size = w->size();
		//this->settings.x = pos.x();
		//this->settings.y = pos.y();
		//this->settings.width = size.width();
		//this->settings.height = size.height();
		this->settingsDirty = true;
		this->nextSettings.x = pos.x();
		this->nextSettings.y = pos.y();
		this->nextSettings.width = size.width();
		this->nextSettings.height = size.height();
	}
}

void OverlaySettingsManager::ApplySettings(bool applyAll, bool withPosition)
{
	std::lock_guard<std::recursive_mutex> l(settingServer.m);
	if (settingServer.widgets.find(tag) == settingServer.widgets.end())
	{
	}
	else
	{
		OverlayMainWindow* mainWindow = settingServer.widgets[tag];
		if ((applyAll || settingsDirty || globalSettings.settingsDirty) && applySettingsNotRecursive && mainWindow)
		{
			applySettingsNotRecursive = false;
			{
				UpdatePositions();
				if ((applyAll || settingsDirty || globalSettings.settingsDirty) && mainWindow)
				{
					std::lock_guard <std::recursive_mutex> w(m);
					applySettingsNotRecursive = false;
					bool useResizeGrip = nextSettings.useResizeGrip && globalSettings.nextSettings.useResizeGrip;
					if (applyAll || useResizeGrip != mainWindow->resizer->isVisible())
					{
						mainWindow->resizer->setVisible(useResizeGrip);
						mainWindow->resizer2->setVisible(useResizeGrip);
						if (!useResizeGrip)
						{
							mainWindow->resizer->setDisabled(true);
							mainWindow->resizer2->setDisabled(true);
							mainWindow->resizer->hide();
							mainWindow->resizer2->hide();
						}
						else
						{
							mainWindow->resizer->setDisabled(false);
							mainWindow->resizer2->setDisabled(false);
							mainWindow->resizer->show();
							mainWindow->resizer2->show();
						}
					}
					bool useDragFilter = nextSettings.useDragFilter && globalSettings.nextSettings.useDragFilter;
					if (applyAll || useDragFilter != mainWindow->filter->useDragFilter)
					{
						mainWindow->filter->useDragFilter = useDragFilter;
					}
					if (applyAll || nextSettings.useAppRegion != settings.useAppRegion)
					{
						mainWindow->filter->useAppRegion = nextSettings.useAppRegion;
					}
					bool useDragMove = nextSettings.useDragMove && globalSettings.nextSettings.useDragMove;
					if (applyAll || useDragMove != mainWindow->filter->useDragMove)
					{
						mainWindow->filter->useDragMove = useDragMove;
					}
					if (applyAll || nextSettings.title != settings.title)
					{
						QByteArray ba = mainWindow->windowTitle().toUtf8();
						std::string title(ba.begin(), ba.end());
						ba = QByteArray::fromStdString(nextSettings.title);
						mainWindow->setWindowTitle(QString::fromUtf8(ba));
						settings.title = nextSettings.title;
					}
					if (applyAll || nextSettings.url != settings.url)
					{
						auto profile = mainWindow->view->page()->profile();
						std::string userInfo = version_string;
						QString useragent;
						useragent = QWebEngineProfile::defaultProfile()->httpUserAgent();
						if (useragent.indexOf("OverlayWindow (") == -1)
						{
							profile->setHttpUserAgent(useragent + " OverlayWindow (" + QString::fromStdString(userInfo) + ")");
						}

						QString s = "var overlayWindowId= \"" + QString::fromStdString(boost::uuids::to_string(tag)) + "\";";
						mainWindow->view->page()->runJavaScript("var overlayWindowId= \"" + QString::fromStdString(boost::uuids::to_string(tag)) + "\";");

						QByteArray ba = QByteArray::fromStdString(nextSettings.url);
						//mainWindow->view->setUrl(QString::fromUtf8(ba));
						if (mainWindow && mainWindow->view && mainWindow->view->page() && mainWindow->view->page()->parent() == mainWindow)
						{
							auto p = mainWindow->view->page();

							p->setBackgroundColor(Qt::GlobalColor::transparent);
							p->runJavaScript(globalBackground + ";var overlayWindowId= \"" + QString::fromStdString(boost::uuids::to_string(tag)) + "\";");
							p->setUrl(QString::fromUtf8(ba));
							mainWindow->view->setPage(p);
						}
						else
						{
							auto p = new OverlayWebEnginePage(mainWindow);

							p->setBackgroundColor(Qt::GlobalColor::transparent);
							p->runJavaScript(globalBackground + ";var overlayWindowId= \"" + QString::fromStdString(boost::uuids::to_string(tag)) + "\";");
							p->setUrl(QString::fromUtf8(ba));
							mainWindow->view->setPage(p);
						}

						//s = "var overlayWindowId= \"" + QString::fromStdString(boost::uuids::to_string(tag)) + "\";";
						//mainWindow->view->page()->runJavaScript("var overlayWindowId= \"" + QString::fromStdString(boost::uuids::to_string(tag)) + "\";");
						mainWindow->view->page()->runJavaScript("var overlayWindowId= \"" + QString::fromStdString(boost::uuids::to_string(tag)) + "\";\nvar overlayWindowInfo = {};");

						//mainWindow->view->page()->setUrl(QString::fromUtf8(ba));
						settings.url = nextSettings.url;

						//mainWindow->view->page()->view()->setMouseTracking(true);
						auto childs = mainWindow->view->children();
						for (auto a : childs) {
							if (a->isWidgetType()) {
								QWidget* widget = (QWidget*)a;
								//widget->setMouseTracking(true);
								a->removeEventFilter(mainWindow->filter);
								a->installEventFilter(mainWindow->filter);
							}
						}
						SetTransparent(mainWindow->view->page()->view(), false);
						SetTransparent(mainWindow, false);
					}
					if (applyAll || nextSettings.fps != settings.fps)
					{
					}

					if (withPosition)
					{
						auto pos = mainWindow->pos();
						auto size = mainWindow->size();
						if (applyAll || nextSettings.width != size.width()
							|| nextSettings.height != size.height())
						{
							mainWindow->resize(nextSettings.width, nextSettings.height);
							std::cout << "resized" << std::endl;
						}
						if (applyAll || nextSettings.x != pos.x()
							|| nextSettings.y != pos.y())
						{
							mainWindow->move(nextSettings.x, nextSettings.y);
						}
					}
					bool useHide = nextSettings.useHide || globalSettings.nextSettings.useHide || MainWindow::windowHideFilter;
					if (applyAll || (useHide != mainWindow->isHidden()))
					{
						if (useHide)
							mainWindow->hide();
						else
							mainWindow->show();
					}
					if (applyAll || nextSettings.zoom != mainWindow->view->page()->zoomFactor())
					{
						mainWindow->view->page()->setZoomFactor(nextSettings.zoom);
					}
					#ifdef _WIN32
					HWND hwnd = (HWND)mainWindow->windowHandle()->winId();
					if (hwnd != INVALID_HANDLE_VALUE)
					{
						if (applyAll || nextSettings.opacity != settings.opacity)
						{
							mainWindow->setWindowOpacity(nextSettings.opacity);
							SetLayeredWindowAttributes(hwnd, 0, (int)(nextSettings.opacity * 255), LWA_ALPHA);
						}
						LONG val = GetWindowLong(hwnd, GWL_EXSTYLE);
						bool useTransparent = nextSettings.useTransparent || globalSettings.nextSettings.useTransparent;
						bool useNoActivate = nextSettings.useNoActivate || globalSettings.nextSettings.useNoActivate;
						if (applyAll || useTransparent != !!(val&WS_EX_TRANSPARENT)
							|| useNoActivate != !!(val&WS_EX_NOACTIVATE))
						{
							if (useTransparent)
								val |= WS_EX_TRANSPARENT;
							else
								val &= ~WS_EX_TRANSPARENT;
							if (useNoActivate)
								val |= WS_EX_NOACTIVATE;
							else
								val &= ~WS_EX_NOACTIVATE;
							SetWindowLong(hwnd, GWL_EXSTYLE, val);
						}
					}
					#endif
					settings = nextSettings;
					settingsDirty = false;
				}

				if (mainWindow && mainWindow->view && mainWindow->view->page())
				{
					//m->view->page()->runJavaScript("var overlayWindowId= \"" + QString::fromStdString(boost::uuids::to_string(id)) + "\";");
				}
				manager->UpdateSetting(&nextSettings, tag);
			}
			applySettingsNotRecursive = true;
		}
	}
}

void OverlaySettingsManager::ApplySettings(const Json::Value& value, bool applyAll)
{
	std::lock_guard<std::recursive_mutex> l(settingServer.m);
	if (settingServer.widgets.find(tag) == settingServer.widgets.end())
	{
		Json::Value cloned = value;
		postToThread([this, applyAll, cloned]
		{
			try {
				settingServer.NewOverlayWindow(tag, cloned);
			}
			catch (...)
			{
				QMessageBox msgBox;
				msgBox.setWindowTitle(QObject::tr("Failed to open overlay window."));
				msgBox.setText(QObject::tr("Failed to open new overlay window.Select another Renderer and restart OverlayProc."));
				msgBox.setStandardButtons(QMessageBox::Ok);
				msgBox.setDefaultButton(QMessageBox::Ok);
				int ret = msgBox.exec();
			}
			//ApplySettings(true);
		});
	}
	else
	{
		postToThread([this, applyAll]
		{
			ApplySettings(applyAll, true);
		});
	}
}

void QOverlaySettingsServer::loadStarted(OverlayMainWindow* m, QWebEngineView* v)
{
	auto childs = v->children();
	for (auto a : childs) {
		if (a->isWidgetType()) {
			QWidget* widget = (QWidget*)a;
			//widget->setMouseTracking(true);
			a->removeEventFilter(m->filter);
			a->installEventFilter(m->filter);
		}
	}
	v->page()->setBackgroundColor(Qt::transparent);
}

Json::Value QOverlaySettingsServer::CaptureOverlayWindow(const boost::uuids::uuid& id) {
	std::lock_guard<std::recursive_mutex> l(this->m);
	Json::Value ret;

	if (widgets.find(id) != widgets.end()) {
		OverlayMainWindow *m = widgets.find(id)->second;
		QPixmap pix;

		//pix = m->view->grab();
		pix = QPixmap::grabWindow(NULL, m->pos().x(), m->pos().y(), m->size().width(), m->size().height());
		
		QBuffer stream;
		pix.save(&stream, "PNG");

		ret["id"] = boost::uuids::to_string(id);
		ret["capture"] = stream.data().toBase64().toStdString();
	}
	else {
		Json::Value error;
		error["error"] = "NotFound";
		return error;
	}
	return ret;
}

Json::Value QOverlaySettingsServer::NewOverlayWindow(const boost::uuids::uuid& id, const Json::Value& value) {
	std::lock_guard<std::recursive_mutex> l(this->m);

	if (widgets.find(id) == widgets.end()) {
		OverlayMainWindow *m = new OverlayMainWindow(manager);
		//m->setAttribute(Qt::WA_DeleteOnClose);
		//m->setAttribute(Qt::WA_InputMethodEnabled, true);

		m->id = id;

		QWebEngineView* v = new OverlayWebEngineView(m);
		//v->setAttribute(Qt::WA_DeleteOnClose);
		//v->setAttribute(Qt::WA_InputMethodEnabled, true);
		m->view = v;
		m->InitSignalSlot();
		m->setWindowTitle("");
		v->setLocale(QLocale::system());
		auto childs = v->children();
		for (auto a : childs) {
			if (a->isWidgetType()) {
				QWidget* widget = (QWidget*)a;
				//widget->setMouseTracking(true);
				widget->setAttribute(Qt::WA_InputMethodEnabled, true);
				a->removeEventFilter(m->filter);
				a->installEventFilter(m->filter);
			}
		}

		QObject::connect(v, &QWebEngineView::loadStarted, qApp, std::move([m, v, id] {

			//v->page()->view()->setMouseTracking(true);
			//v->page()->setBackgroundColor(globalBackground);
			v->page()->view()->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
			SetTransparent(v->page()->view(), false);

			auto childs = v->children();
			for (auto a : childs) {
				if (a->isWidgetType()) {
					QWidget* widget = (QWidget*)a;
					//widget->setMouseTracking(true);
					widget->setAttribute(Qt::WA_InputMethodEnabled, true);
					a->removeEventFilter(m->filter);
					a->installEventFilter(m->filter);
				}
			}
			SetTransparent(m, false);
			SetTransparent(v->page()->view(), false);
			//v->page()->setBackgroundColor(globalBackground);
			v->hide();
			auto profile = m->view->page()->profile();
			m->view->page()->runJavaScript("var overlayWindowId= \"" + QString::fromStdString(boost::uuids::to_string(id)) + "\";\nvar overlayWindowInfo = {};");

		}));
		QObject::connect(v, &QWebEngineView::loadFinished, qApp, std::move([m, v, this, id] {
			m->view->page()->runJavaScript("var overlayWindowId= \"" + QString::fromStdString(boost::uuids::to_string(id)) + "\";\nvar overlayWindowInfo = {};");
			auto childs = v->children();
			for (auto a : childs) {
				if (a->isWidgetType()) {
					QWidget* widget = (QWidget*)a;
					//widget->setMouseTracking(true);
					a->removeEventFilter(m->filter);
					a->installEventFilter(m->filter);
				}
			}
			SetTransparent(m, false);
			SetTransparent(v->page()->view(), false);
			v->show();
			//v->page()->setBackgroundColor(globalBackground);
			v->page()->runJavaScript(globalBackground);

			this->m.lock();
			UpdateTitle(id, v->title().toStdString());
			manager->UpdateSetting(&settings[id].nextSettings, id);
			this->m.unlock();
			QFile file;
			file.setFileName(":/appregion.js");
			file.open(QIODevice::ReadOnly);
			QString jQuery = file.readAll();
			file.close();
			jQuery.append("\nvar overlayWindowId= \"" + QString::fromStdString(boost::uuids::to_string(id)) + "\";");

			v->page()->runJavaScript(jQuery);
		}));
		QObject::connect(v, &QWebEngineView::titleChanged, qApp, std::move([m, v, this, id] {
			this->m.lock();
			UpdateTitle(id, v->title().toStdString());
			manager->UpdateSetting(&settings[id].nextSettings, id);
			this->m.unlock();
		}));
		v->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
		m->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
		m->setCentralWidget(v);


		SetTransparent(m, false);

		m->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::NoDropShadowWindowHint | Qt::FramelessWindowHint | Qt::Dialog | Qt::Tool);

		widgets[id] = m;
		m->showMinimized();
		m->hide();
		settings[id].UpdateSettings(id, value, true);
		int x, y, width, height;
		x = settings[id].nextSettings.x;
		y = settings[id].nextSettings.y;
		width = settings[id].nextSettings.width;
		height = settings[id].nextSettings.height;
		m->move(x,y);
		m->resize(width,height);
		m->show();

		auto profile = m->view->page()->profile();
		std::string userInfo = version_string;
		QString useragent;
		useragent = QWebEngineProfile::defaultProfile()->httpUserAgent();
		if (useragent.indexOf("OverlayWindow (") == -1)
		{
			profile->setHttpUserAgent(useragent + " OverlayWindow (" + QString::fromStdString(userInfo) + ")");
		}

		QString s = "var overlayWindowId= \"" + QString::fromStdString(boost::uuids::to_string(id)) + "\";";
		std::string z = s.toStdString();
		m->view->page()->runJavaScript("var overlayWindowId= \"" + QString::fromStdString(boost::uuids::to_string(id)) + "\";");

	}
	else
	{
		settings[id].UpdateSettings(id, value);
	}

	settings[id].ApplySettings(true);
	Json::Value ret = value;
	ret["id"] = boost::uuids::to_string(id);
	//manager->UpdateSetting(&settings[id].nextSettings, id);
	return ret;
}

void QOverlaySettingsServer::CloseOverlayWindow(const boost::uuids::uuid& id) {
	postToThread([this, id]
	{
		std::lock_guard<std::recursive_mutex> l(this->m);

		if (widgets.find(id) != widgets.end()) {
			widgets[id]->close();
			widgets[id]->deleteLater();
			widgets.erase(id);
			settings.erase(id);
		}
	});
}
