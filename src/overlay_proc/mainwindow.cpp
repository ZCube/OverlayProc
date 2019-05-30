#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QListWidget>
#include <QListWidgetItem>
#include <QCloseEvent>
#include <QSettings>
#include <QWebEngineView>
#include <boost/lexical_cast.hpp>

#include "overlaymainwindow.h"
#include <json/json.h>
#include <settings/settings.h>
#include "utility.h"
#include "settings_impl.h"
#include <QtGlobal>
#include <QThread>
#include <QTimer>
#include <QMenu>
#include <QColorDialog>
#include <QUrlQuery>

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#include <psapi.h>
#include <tlhelp32.h>
#endif
//#pragma comment(lib, "shlwapi.lib")
//#pragma comment(lib, "psapi.lib")
#include <boost/algorithm/string.hpp>  
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid_io.hpp>

extern std::string QtWebEngineVersion;
extern std::string ChromeVersion;

QString globalBackground = "document.body.style.background = \"transparent\"";

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

static void InitKeyMap();

MainWindow::MainWindow(uint16_t server_port, QWidget *parent) :
	QMainWindow(parent),
	QAbstractNativeEventFilter(),
	ui(new Ui::MainWindow)
{
	InitKeyMap();
	ui->setupUi(this);
	setOnlyGlobal = false;
	setOnly = false;

	InitValue();
	loadSettings();
	QApplication::instance()->installNativeEventFilter(this);
	setWindowFlags(
		Qt::Window |
		Qt::WindowTitleHint |
		Qt::WindowSystemMenuHint |
		Qt::WindowCloseButtonHint
	);
	ui->urlListWidget->installEventFilter(this);
	ui->newURL->installEventFilter(this);

	if (QtWebEngineVersion.size() > 0)
	{
		ui->label_16->setText(ui->label_16->text().replace("Qt 5.7.1 WebEngine", QtWebEngineVersion.c_str()));
	}
	if (ChromeVersion.size() > 0)
	{
		ui->label_16->setText(ui->label_16->text().replace("Chromium 49", ChromeVersion.c_str()));
	}

	// context menu
	menu = new QMenu(this);
	managerAction = new QAction(QObject::tr("manager"), menu);
	menu->addAction(managerAction);
	QObject::connect(managerAction, SIGNAL(triggered()), this, SLOT(showManager()));

	exitAction = new QAction(QObject::tr("exit"), menu);
	menu->addAction(exitAction);
	QObject::connect(exitAction, SIGNAL(triggered()), this, SLOT(exitManager()));

	icon = new QSystemTrayIcon(this);
	QObject::connect(icon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(slotActivated(QSystemTrayIcon::ActivationReason)));

	icon->setIcon(QIcon(":/images/icon.png"));
	icon->setVisible(true);
	icon->setContextMenu(menu);
	icon->show();
	
	// copy data widget
	cdw = new CopyDataWidget(this, this);
	cdw->setWindowTitle("OverlayProcWMCOPYDATA");
	cdw->setWindowFlags(Qt::NoDropShadowWindowHint | Qt::FramelessWindowHint | Qt::Tool);
	cdw->showMinimized();
	cdw->hide();

	//
	server = new WebSocketServer(this, QHostAddress::LocalHost, server_port, this);

	{
		Json::Value value;
		value["cmd"] = "get_urllist";
		Broadcast(0, QString::fromStdString(value.toStyledString()));
	}

	// timer
	timer = new QTimer(this);
	timer->setInterval(1000);
	timer->setSingleShot(false);
	connect(timer, SIGNAL(timeout()), this, SLOT(on_timer()));
	timer->start();

}

bool MainWindow::prevWindowHideFilter = false;
bool MainWindow::windowHideFilter = false;


#ifdef _WIN32
ULONG ProcIDFromWnd(HWND hwnd)
{
	ULONG idProc;
	GetWindowThreadProcessId(hwnd, &idProc);
	return idProc;
}
HWND GetWinHandle(ULONG pid)
{
	HWND tempHwnd = FindWindow(NULL, NULL); 

	while (tempHwnd != NULL)
	{
		if (GetParent(tempHwnd) == NULL)
			if (pid == ProcIDFromWnd(tempHwnd))
				return tempHwnd;
		tempHwnd = GetWindow(tempHwnd, GW_HWNDNEXT);
	}
	return NULL;
}
#endif

bool IsVisible(std::list<std::wstring> processes, int mode)
{
	if (processes.size() == 0)
		return true;

#ifdef _WIN32
	DWORD pid;
	PROCESSENTRY32W pepid;
	bool found = false;
	bool visible = false; // at one
	bool active = false;
	std::list<DWORD> processIDs;
	{
		HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		PROCESSENTRY32W pe = { 0 };
		pe.dwSize = sizeof(PROCESSENTRY32W);
		if (Process32FirstW(h, &pe)) {
			do {
				boost::filesystem::path p(pe.szExeFile);
				std::wstring f = p.filename().wstring();
				boost::algorithm::to_lower(f);
				for (auto j = processes.begin(); j != processes.end(); ++j)
				{
					boost::filesystem::path p2(*j);
					std::wstring f2 = p2.filename().wstring();
					boost::algorithm::to_lower(f2);
					if (f == f2)
					{
						found = true;
						pepid = pe;
						pid = pe.th32ProcessID;
						processIDs.push_back(pid);
					}
				}
			} while (Process32NextW(h, &pe));
		}

		CloseHandle(h);
	}
	for (auto i = processIDs.begin();
		i != processIDs.end();
		++i)
	{
		HWND hwnd = GetWinHandle(*i);
		RECT rect = { -1, };
		GetClientRect(hwnd, &rect);
		bool invisible = (rect.left == 0 && rect.top == 0 && rect.right == 0 && rect.bottom == 0) || !IsWindowVisible(hwnd);
		visible = visible | (!invisible);
	}
	DWORD processID = 0;
	HWND hwnd = GetForegroundWindow();
	GetWindowThreadProcessId(hwnd, &processID);
	active = (std::find(processIDs.begin(), processIDs.end(), processID) != processIDs.end()) | (processID == GetCurrentProcessId());
	//GetActiveWindow() == //if (activeFilter)
	//if(ui->radioButtion_Focus->isChecked())
	switch (mode)
	{
	case 0:
		MainWindow::windowHideFilter = MainWindow::windowHideFilter || !active;
		break;
	case 1:
		MainWindow::windowHideFilter = MainWindow::windowHideFilter || !visible;
		break;
	}
#endif
	return MainWindow::windowHideFilter;
}
//
//BOOL CALLBACK EnumWindowCallBack2(HWND hwnd, LPARAM lParam)
//{
//	if (!::IsIconic(hwnd)) {
//		return TRUE;
//	}
//	int length = ::GetWindowTextLength(hwnd);
//	if (0 == length) return TRUE;
//
//	std::list<std::wstring>* processes = (std::list<std::wstring>*)lParam;
//	DWORD pid;
//	bool found = false;
//	if (GetWindowThreadProcessId(hwnd, &pid))
//	{
//		HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
//		if (processHandle)
//		{
//			wchar_t szPath[1024] = { 0 };
//			GetModuleFileNameExW(processHandle, NULL, szPath, 1023);
//			boost::filesystem::path p(szPath);
//			std::wstring f = p.filename().wstring();
//			boost::algorithm::to_lower(f);
//
//			for (auto j = processes->begin(); j != processes->end(); ++j)
//			{
//				boost::filesystem::path p2(*j);
//				std::wstring f2 = p2.filename().wstring();
//				boost::algorithm::to_lower(f2);
//				if (f == f2)
//				{
//					found = true;
//					break;
//				}
//			}
//			CloseHandle(processHandle);
//		}
//		if (found)
//		{
//			MainWindow::windowHideFilter = MainWindow::windowHideFilter || !IsWindowVisible(hwnd);
//		}
//	}
//	return TRUE;
//}


void MainWindow::on_auto_hide_toggled(bool checked)
{
	prevWindowHideFilter = windowHideFilter;
	windowHideFilter = false;

	if (prevWindowHideFilter != windowHideFilter)
	{
		std::lock_guard<std::recursive_mutex> l(settingServer.m);
		globalSettings.nextSettings.useHide = !ui->show_all->isChecked() | windowHideFilter;
		globalSettings.CheckDirty();

		for (auto i = settingServer.settings.begin(); i != settingServer.settings.end(); ++i) {
			i->second.ApplySettings();
		}

		globalSettings.settings = globalSettings.nextSettings;
		globalSettings.CheckDirty();
	}
}

void MainWindow::on_timer()
{
	prevWindowHideFilter = windowHideFilter;
	windowHideFilter = false;

	if (ui->auto_hide->isChecked())
	{
		windowHideFilter = false;

		std::list<std::wstring> processes;
		for (int i = 0; i < ui->processesListWidget->count(); ++i)
		{
			QListWidgetItem* item = ui->processesListWidget->item(i);
			std::wstring process = item->text().toLower().toStdWString();
			processes.push_back(process);
		}

		if (processes.size() == 0)
		{
			windowHideFilter = false;
		}
		std::list<HWND> hwndList;
		//	BOOL b = EnumWindows(::EnumWindowCallBack2, (LPARAM)&processes);

		bool b = IsVisible(processes, ui->hide_filter_condition->currentIndex());
	}
	
	if (prevWindowHideFilter != windowHideFilter)
	{
		std::lock_guard<std::recursive_mutex> l(settingServer.m);
		globalSettings.nextSettings.useHide = !ui->show_all->isChecked() | windowHideFilter;
		globalSettings.CheckDirty();

		for (auto i = settingServer.settings.begin(); i != settingServer.settings.end(); ++i) {
			i->second.ApplySettings();
		}

		globalSettings.settings = globalSettings.nextSettings;
		globalSettings.CheckDirty();
	}
}

void MainWindow::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::WindowStateChange) {
		if (isMinimized())
			this->hide();
		event->ignore();
	}
}

void MainWindow::showEvent(QShowEvent *event) {
	QSettings settings;
	restoreGeometry(settings.value("mainWindowGeometry").toByteArray());
	restoreState(settings.value("mainWindowState").toByteArray());
};

void MainWindow::hideEvent(QHideEvent *event) {
};

void MainWindow::SetURLList(Json::Value& value)
{
	Json::Value list = value["URLList"];

	ui->urlListWidget->clear();
	std::set<std::string> urls;
	for (auto i = list.begin(); i != list.end(); ++i)
	{
		std::string url = (*i)["URL"].asString();
		std::string title = (*i)["Title"].asString();

		QUrl u = QString::fromStdString(url);
		if (u.hasQuery())
		{
			QUrlQuery query(u.query());
			query = query;
			auto items = query.queryItems();
			for (auto q = items.begin(); q != items.end(); ++q)
			{
				q->first;
				q->second;
				//qDebug() << q->first << " " << q->second;
			}
		}

		if (urls.find(url) == urls.end())
		{
			QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(title), ui->urlListWidget);
			item->setData(Qt::UserRole, QString::fromStdString(url));
			urls.insert(url);
		}
	}

	Json::Value vars = value["Vars"];
	for (auto i = vars.begin(); i != vars.end(); ++i)
	{
	}
	ui->urlListWidget->sortItems();
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == event->FocusIn)
	{
		if (object == ui->urlListWidget)
		{
			
		}
		else if (object == ui->newURL)
		{
			QList<QListWidgetItem*> items = ui->urlListWidget->selectedItems();
			foreach(QListWidgetItem * item, items)
			{
				ui->urlListWidget->setItemSelected(item, false);
			}
			return false;
		}
		return false;
	}
	else
	{
		return false;
	}
}

bool MainWindow::nativeEventFilter(const QByteArray& eventType, void* message, long*)
{
#ifdef _WIN32
	if (eventType == "windows_generic_MSG") {
		MSG* msg = static_cast<MSG*>(message);
		if (msg->message == WM_HOTKEY) {
			qDebug() << "WM_HOTKEY !" << " " << msg->wParam;
			switch (msg->wParam)
			{
			case 0:
			{
			}
			break;
			case 1:
				ui->show_all->setChecked(!ui->show_all->isChecked());
			break;
			case 2: // manager
				if (this->isHidden())
					this->showNormal();
				else
					this->hide();
				break;
			case 3: // click_through_all_shortcut
				ui->click_through_all->setChecked(!ui->click_through_all->isChecked());
				break;
			case 4: // no_focus_all_shortcut
				ui->no_focus_all->setChecked(!ui->no_focus_all->isChecked());
				break;
			case 5: // draggable_all_shortcut
				ui->draggable_all->setChecked(!ui->draggable_all->isChecked());
				break;
			}
			return true;
		}
	}
#endif
	return false;
}

void MainWindow::InitValue()
{
	//item->
	setOnly = true;
	ui->title->setText("");
	ui->url->setText("");
	ui->opacity_spin->setRange(0.0, 1.0);
	ui->fps_spin->setRange(1, 60);
	ui->zoom_spin->setRange(0.5, 2.0);

	ui->opacity->setRange(0, 100);
	ui->fps->setRange(1, 60);
	ui->zoom->setRange(5, 20);
	setOnly = false;

	ui->groupBox->setDisabled(true);
	//ui->title->setDisabled(true);
	//ui->url->setDisabled(true);
	//ui->click_through->setDisabled(true);
	//ui->draggable->setDisabled(true);
	//ui->no_focus->setDisabled(true);
	//ui->resizeable->setDisabled(true);
	//ui->show->setDisabled(true);
	//ui->opacity->setDisabled(true);
	//ui->fps->setDisabled(true);
	//ui->zoom->setDisabled(true);
	//ui->x->setDisabled(true);
	//ui->y->setDisabled(true);
	//ui->width->setDisabled(true);
	//ui->height->setDisabled(true);
}

MainWindow::~MainWindow()
{
	saveSettings();
	boost::filesystem::path p = "overlays.json";

	Json::Value val = settingServer.get_all();
	try
	{
#ifdef _WIN32
		std::ofstream fout(p.wstring());
#else
		std::ofstream fout(p.string().c_str());
#endif
		Json::StyledStreamWriter writer;
		writer.write(fout, val);
		fout.close();
	}
	catch (std::exception& e)
	{
	}
	//settingServer.close_all();
	timer->stop();
	delete ui;
}

void MainWindow::SendTo(ClientID sender, int code, const QString & data)
{
#ifdef _WIN32
	if (cdw && sender.hwnd != 0)
	{
		cdw->SendTo(sender, code, data);
	}
#endif
	if (server && sender.socket != nullptr)
	{
		server->SendTo(sender, code, data);
	}
}

void MainWindow::Broadcast(int code, const QString & data)
{
	if (cdw)
	{
		cdw->Broadcast(code, data);
	}
	if (server)
	{
		server->Broadcast(code, data);
	}
}

void MainWindow::HandleData(ClientID cid, int code, const QString& message)
{
	postToThread([this, cid, code, message]
	{
		std::string s = message.toStdString();
		Json::Reader reader;
		Json::Value root;
		if (reader.parse(s, root)) {
			Json::Value null;
			Json::Value cmd;
			Json::Value ret;
			Json::FastWriter writer;
			try {
				if ((cmd = root.get("cmd", null)) != null) {
					Json::Value& value = root["value"];
					std::string cmdString = cmd.asString();
					if (cmdString == "urllist")
					{
						SetURLList(value);
						Broadcast(code, message);
					}
					else if (cmdString == "manager")
					{
						this->showManager();
					}
					else if (cmdString == "stop")
					{
						this->exitManager();
					}
					else if (cmdString == "check")
					{
						//std::cout << s << " " << cid.Dump() << std::endl;
					}
					else if (cmdString == "set")
					{
						std::cout << s << " " << cid.Dump() << " " << value << std::endl;
						ret["cmd"] = cmdString;
						ret["value"] = settingServer.set(value);
					}
					else if (cmdString == "capture")
					{
						ret["cmd"] = cmdString;
						ret["value"] = settingServer.capture(value);
					}
				}
				//	else if (cmdString == "set")
				//	{
				//		ret["cmd"] = cmdString;
				//		ret["value"] = settingServer.set(value);
				//	}
				//	else if (cmdString == "set_all")
				//	{
				//		ret["cmd"] = cmdString;
				//		ret["value"] = settingServer.set_all(value);
				//	}
				//	else if (cmdString == "get")
				//	{
				//		ret["cmd"] = cmdString;
				//		ret["value"] = settingServer.get(value);
				//	}
				//	else if (cmdString == "get_all")
				//	{
				//		ret["cmd"] = cmdString;
				//		ret["value"] = settingServer.get_all();
				//	}
				//	else if (cmdString == "capture")
				//	{
				//		ret["cmd"] = cmdString;
				//		ret["value"] = settingServer.capture(value);
				//	}
				//	else if (cmdString == "close")
				//	{
				//		ret["cmd"] = cmdString;
				//		ret["value"] = settingServer.close(value);
				//	}
				//	else if (cmdString == "close_all")
				//	{
				//		ret["cmd"] = cmdString;
				//		ret["value"] = settingServer.close_all();
				//	}
				//}
				std::string content = "";
				if (ret == null)
				{
					content = ".";
				}
				else
				{
					content = writer.write(ret);
				}
				SendTo(cid, code, QString::fromStdString(content));
			}
			catch (...)
			{
			}
		}
	});
}

void MainWindow::on_listWidget_currentRowChanged(int currentRow)
{
	QListWidgetItem* currentItem = ui->listWidget->currentItem();
	if (currentItem != nullptr)
	{
		QVariant data = currentItem->data(Qt::UserRole);
		QString idStr = data.toString();
		auto id = boost::lexical_cast<boost::uuids::uuid>(idStr.toStdString());
		settingServer.m.lock();
		auto i = settingServer.settings.find(id);
		if (i != settingServer.settings.end())
		{
			UpdateSetting(&i->second.settings, id);
		}
		settingServer.m.unlock();
	}
}

void MainWindow::closeEvent(QCloseEvent * event)
{
	QSettings settings;
	settings.setValue("mainWindowGeometry", saveGeometry());
	settings.setValue("mainWindowState", saveState());

	saveSettings();
	//event->ignore();
#ifdef _WIN32
	this->hide();
	event->ignore();
#else
	QApplication::instance()->quit();
#endif
};

void MainWindow::on_opacity_valueChanged(int value)
{
	if (setOnly)
		return;
	ui->opacity_spin->setValue(value / 100.0);
}

void MainWindow::on_opacity_sliderReleased()
{

}

void MainWindow::on_zoom_sliderPressed()
{
}

void MainWindow::on_zoom_sliderReleased()
{
}

void MainWindow::on_zoom_valueChanged(int value)
{
    if (setOnly)
        return;
    ui->zoom_spin->setValue(value / 10.0);
}

void MainWindow::on_fps_valueChanged(int value)
{
	if (setOnly)
		return;
	ui->fps_spin->setValue(value);
}

void MainWindow::on_opacity_spin_valueChanged(double arg1)
{
	ui->opacity->setValue(arg1 * 100.0);
	if (setOnly)
		return;

	NotifySetting();
}

void MainWindow::on_zoom_spin_valueChanged(double arg1)
{
	ui->zoom->setValue(arg1 * 10.0);
	if (setOnly)
		return;

	NotifySetting();
}

void MainWindow::on_fps_spin_valueChanged(int arg1)
{
	ui->fps->setValue(arg1);
	if (setOnly)
		return;

	NotifySetting();
}

void MainWindow::on_resizeable_toggled(bool checked)
{
	if (setOnly)
		return;

	NotifySetting();
}

void MainWindow::on_draggable_toggled(bool checked)
{
	if (setOnly)
		return;

	NotifySetting();
}

void MainWindow::on_appregion_toggled(bool checked)
{
	if (setOnly)
		return;

	NotifySetting();
}

void MainWindow::on_show_toggled(bool checked)
{
	if (setOnly)
		return;

	NotifySetting();
}

void MainWindow::on_click_through_toggled(bool checked)
{
	if (setOnly)
		return;

	NotifySetting();
}

void MainWindow::on_no_focus_toggled(bool checked)
{
	if (setOnly)
		return;

	NotifySetting();
}
//
//
//void MainWindow::on_x_editingFinished()
//{
//    if (setOnly)
//        return;
//
//    NotifySetting();
//}
//
//void MainWindow::on_width_editingFinished()
//{
//    if (setOnly)
//        return;
//
//    NotifySetting();
//}
//
//void MainWindow::on_y_editingFinished()
//{
//    if (setOnly)
//        return;
//
//    NotifySetting();
//}
//
//void MainWindow::on_height_editingFinished()
//{
//    if (setOnly)
//        return;
//
//    NotifySetting();
//}


void MainWindow::NotifySetting()
{
	QListWidgetItem* currentItem = ui->listWidget->currentItem();
	if (currentItem != nullptr)
	{
		QVariant data = currentItem->data(Qt::UserRole);
		QString idStr = data.toString();
		auto id = boost::lexical_cast<boost::uuids::uuid>(idStr.toStdString());
		settingServer.m.lock();
		auto i = settingServer.settings.find(id);
		if (i != settingServer.settings.end())
		{
			setOnly = true;
			i->second.settingsDirty = true;
			NotifySetting(&i->second.nextSettings, id);
			i->second.nextSettings.x = i->second.settings.x;
			i->second.nextSettings.y = i->second.settings.y;
			i->second.nextSettings.width = i->second.settings.width;
			i->second.nextSettings.height = i->second.settings.height;
			i->second.ApplySettings();
			setOnly = false;
		}
		settingServer.m.unlock();
	}
}

void MainWindow::NotifySetting(OverlaySettings* set, const boost::uuids::uuid& id)
{
	QListWidgetItem* item = nullptr;
	for (int i = 0; i < ui->listWidget->count(); ++i)
	{
		QListWidgetItem* _item = ui->listWidget->item(i);
		if (_item)
		{
			QVariant data = _item->data(Qt::UserRole);
			QString idStr = data.toString();
			if (boost::uuids::to_string(id) == idStr.toStdString())
			{
				item = _item;
				break;
			}
		}
	}
	QListWidgetItem* currentItem = ui->listWidget->currentItem();
	if (item == nullptr)
	{
		QString title;
		{
			QByteArray ba = QByteArray::fromStdString(set->title);
			title = QString::fromUtf8(ba);
		}
		QListWidgetItem* item = nullptr;
		item = new QListWidgetItem(title, ui->listWidget);
		item->setData(Qt::UserRole, QString::fromStdString(boost::uuids::to_string(id)));
	}
	else if (currentItem == item) {
		QVariant data = currentItem->data(Qt::UserRole);
		{
			//set->x = ui->x->text().toInt();
			//set->y = ui->y->text().toInt();
			//set->width = ui->width->text().toInt();
			//set->height = ui->height->text().toInt();

			set->opacity = ui->opacity_spin->value();
			set->zoom = ui->zoom_spin->value();
			set->fps = ui->fps_spin->value();

			{
				QString str = ui->title->text();
				set->title = str.toUtf8().toStdString();
			}
			{
				QString str = ui->url->text();
				set->url = str.toUtf8().toStdString();
			}

			set->useTransparent = ui->click_through->isChecked();
			set->useDragMove = set->useDragFilter = ui->draggable->isChecked();
			set->useNoActivate = ui->no_focus->isChecked();
			set->useResizeGrip = ui->resizeable->isChecked();
			set->useHide = !ui->show->isChecked();
			set->useAppRegion = ui->appregion->isChecked();
		}
	}
}

void MainWindow::ResetPosition(OverlaySettings* set, const boost::uuids::uuid& id)
{
	QListWidgetItem* item = nullptr;
	for (int i = 0; i < ui->listWidget->count(); ++i)
	{
		QListWidgetItem* _item = ui->listWidget->item(i);
		if (_item)
		{
			QVariant data = _item->data(Qt::UserRole);
			QString idStr = data.toString();
			if (boost::uuids::to_string(id) == idStr.toStdString())
			{
				item = _item;
				break;
			}
		}
	}
	QListWidgetItem* currentItem = ui->listWidget->currentItem();
	if (item == nullptr)
	{
		QString title;
		{
			QByteArray ba = QByteArray::fromStdString(set->title);
			title = QString::fromUtf8(ba);
		}
		QListWidgetItem* item = nullptr;
		item = new QListWidgetItem(title, ui->listWidget);
		item->setData(Qt::UserRole, QString::fromStdString(boost::uuids::to_string(id)));
	}
	else if (currentItem == item) {
		QVariant data = currentItem->data(Qt::UserRole);
		{
			set->x = 0;
			set->y = 0;
			set->width = 300;
			set->height = 300;
		}
	}
}

void MainWindow::UpdateSetting(OverlaySettings* set, const boost::uuids::uuid& id)
{
	if (setOnly)
		return;
	QListWidgetItem* item = nullptr;
	for (int i = 0; i < ui->listWidget->count(); ++i)
	{
		QListWidgetItem* _item = ui->listWidget->item(i);
		if (_item)
		{
			QVariant data = _item->data(Qt::UserRole);
			QString idStr = data.toString();
			if (boost::uuids::to_string(id) == idStr.toStdString())
			{
				item = _item;
				break;
			}
		}
	}
	QListWidgetItem* currentItem = ui->listWidget->currentItem();
	setOnly = true;

	if (item == nullptr)
	{
		QString title;
		{
			QByteArray ba = QByteArray::fromStdString(set->title);
			title = QString::fromUtf8(ba);
		}
		QListWidgetItem* item = nullptr;
		item = new QListWidgetItem(title, ui->listWidget);
		item->setData(Qt::UserRole, QString::fromStdString(boost::uuids::to_string(id)));
	}
	else if (currentItem == item) {
		QVariant data = currentItem->data(Qt::UserRole);
		QString idStr = data.toString();
		{

			ui->groupBox->setDisabled(false);
			//ui->title->setDisabled(false);
			//ui->url->setDisabled(false);
			//ui->click_through->setDisabled(false);
			//ui->draggable->setDisabled(false);
			//ui->no_focus->setDisabled(false);
			//ui->resizeable->setDisabled(false);
			//ui->show->setDisabled(false);
			//ui->opacity->setDisabled(false);
			//ui->fps->setDisabled(false);
			//ui->zoom->setDisabled(false);

			//ui->x->setText(QString::number(set->x));
			//ui->y->setText(QString::number(set->y));
			//ui->width->setText(QString::number(set->width));
			//ui->height->setText(QString::number(set->height));

			ui->opacity_spin->setValue(set->opacity);
			ui->zoom_spin->setValue(set->zoom);
			ui->fps_spin->setValue(set->fps);

			{
				QByteArray ba = QByteArray::fromStdString(set->title);
				QString str = QString::fromUtf8(ba);
				ui->title->setText(str);

				if (item != nullptr)
				{
					item->setText(str);
				}
			}
			{
				QByteArray ba = QByteArray::fromStdString(set->url);
				ui->url->setText(QString::fromUtf8(ba));
			}

			ui->click_through->setChecked(set->useTransparent);
			ui->appregion->setChecked(set->useAppRegion);
			ui->draggable->setChecked(set->useDragMove && set->useDragFilter);
			ui->no_focus->setChecked(set->useNoActivate);
			ui->resizeable->setChecked(set->useResizeGrip);
			ui->show->setChecked(!set->useHide);
		}
	}
	else
	{
		{
			{
				QByteArray ba = QByteArray::fromStdString(set->title);
				if (item != nullptr)
				{
					item->setText(QString::fromUtf8(ba));
				}
			}
		}
	}
	setOnly = false;
}

void MainWindow::on_closeButton_clicked()
{
	QListWidgetItem* currentItem = ui->listWidget->currentItem();
	if (currentItem != nullptr)
	{
		QVariant data = currentItem->data(Qt::UserRole);
		QString idStr = data.toString();
		auto id = boost::lexical_cast<boost::uuids::uuid>(idStr.toStdString());
		settingServer.m.lock();
		settingServer.CloseOverlayWindow(id);
		settingServer.m.unlock();
		delete currentItem;
		InitValue();
		on_listWidget_currentRowChanged(ui->listWidget->currentRow());
	}
}

std::map<int, Qt::Key> keyMap;
std::map<Qt::Key, int> keyMapReverse;

static void InitKeyMap()
{
#ifdef _WIN32
	keyMap[VK_BACK] = Qt::Key_Backspace;
	keyMap[VK_TAB] = Qt::Key_Tab;
	keyMap[VK_RETURN] = Qt::Key_Return;
	keyMap[VK_SHIFT] = Qt::Key_Shift;
	keyMap[VK_CONTROL] = Qt::Key_Control;
	keyMap[VK_MENU] = Qt::Key_Menu;
	keyMap[VK_PAUSE] = Qt::Key_Pause;
	keyMap[VK_ESCAPE] = Qt::Key_Escape;
	keyMap[VK_SPACE] = Qt::Key_Space;
	keyMap[VK_PRIOR] = Qt::Key_PageUp;
	keyMap[VK_NEXT] = Qt::Key_PageDown;
	keyMap[VK_END] = Qt::Key_End;
	keyMap[VK_HOME] = Qt::Key_Home;
	keyMap[VK_LEFT] = Qt::Key_Left;
	keyMap[VK_UP] = Qt::Key_Up;
	keyMap[VK_RIGHT] = Qt::Key_Right;
	keyMap[VK_DOWN] = Qt::Key_Down;
	keyMap[VK_PRINT] = Qt::Key_Print;
	keyMap[VK_INSERT] = Qt::Key_Insert;
	keyMap[VK_DELETE] = Qt::Key_Delete;
	keyMap[VK_HELP] = Qt::Key_Help;
	keyMap['0'] = Qt::Key_0;
	keyMap['1'] = Qt::Key_1;
	keyMap['2'] = Qt::Key_2;
	keyMap['3'] = Qt::Key_3;
	keyMap['4'] = Qt::Key_4;
	keyMap['5'] = Qt::Key_5;
	keyMap['6'] = Qt::Key_6;
	keyMap['7'] = Qt::Key_7;
	keyMap['8'] = Qt::Key_8;
	keyMap['9'] = Qt::Key_9;
	keyMap['A'] = Qt::Key_A;
	keyMap['B'] = Qt::Key_B;
	keyMap['C'] = Qt::Key_C;
	keyMap['D'] = Qt::Key_D;
	keyMap['E'] = Qt::Key_E;
	keyMap['F'] = Qt::Key_F;
	keyMap['G'] = Qt::Key_G;
	keyMap['H'] = Qt::Key_H;
	keyMap['I'] = Qt::Key_I;
	keyMap['J'] = Qt::Key_J;
	keyMap['K'] = Qt::Key_K;
	keyMap['L'] = Qt::Key_L;
	keyMap['M'] = Qt::Key_M;
	keyMap['N'] = Qt::Key_N;
	keyMap['O'] = Qt::Key_O;
	keyMap['P'] = Qt::Key_P;
	keyMap['Q'] = Qt::Key_Q;
	keyMap['R'] = Qt::Key_R;
	keyMap['S'] = Qt::Key_S;
	keyMap['T'] = Qt::Key_T;
	keyMap['U'] = Qt::Key_U;
	keyMap['V'] = Qt::Key_V;
	keyMap['W'] = Qt::Key_W;
	keyMap['X'] = Qt::Key_X;
	keyMap['Y'] = Qt::Key_Y;
	keyMap['Z'] = Qt::Key_Z;
	keyMap[VK_ADD] = Qt::Key_Plus;
	keyMap[VK_SUBTRACT] = Qt::Key_Minus;
	keyMap[VK_DECIMAL] = Qt::Key_Period;
	keyMap[VK_F1] = Qt::Key_F1;
	keyMap[VK_F2] = Qt::Key_F2;
	keyMap[VK_F3] = Qt::Key_F3;
	keyMap[VK_F4] = Qt::Key_F4;
	keyMap[VK_F5] = Qt::Key_F5;
	keyMap[VK_F6] = Qt::Key_F6;
	keyMap[VK_F7] = Qt::Key_F7;
	keyMap[VK_F8] = Qt::Key_F8;
	keyMap[VK_F9] = Qt::Key_F9;
	keyMap[VK_F10] = Qt::Key_F10;
	keyMap[VK_F11] = Qt::Key_F11;
	keyMap[VK_F12] = Qt::Key_F12;
	keyMap[VK_F13] = Qt::Key_F13;
	keyMap[VK_F14] = Qt::Key_F14;
	keyMap[VK_F15] = Qt::Key_F15;
	keyMap[VK_F16] = Qt::Key_F16;
	keyMap[VK_F17] = Qt::Key_F17;
	keyMap[VK_F18] = Qt::Key_F18;
	keyMap[VK_F19] = Qt::Key_F19;
	keyMap[VK_F20] = Qt::Key_F20;
	keyMap[VK_F21] = Qt::Key_F21;
	keyMap[VK_F22] = Qt::Key_F22;
	keyMap[VK_F23] = Qt::Key_F23;
	keyMap[VK_F24] = Qt::Key_F24;
	keyMap[VK_NUMLOCK] = Qt::Key_NumLock;
	keyMap[VK_SCROLL] = Qt::Key_ScrollLock;
#endif

	for (auto a : keyMap)
	{
		keyMapReverse[a.second] = a.first;
	}
}

static int getModifier(int keycode)
{
	int mod = 0;
#ifdef _WIN32
	if ((keycode & Qt::META) == Qt::META)
		mod |= MOD_WIN;
	if ((keycode & Qt::ALT) == Qt::ALT)
		mod |= MOD_ALT;
	if ((keycode & Qt::CTRL) == Qt::CTRL)
		mod |= MOD_CONTROL;
	if ((keycode & Qt::SHIFT) == Qt::SHIFT)
		mod |= MOD_SHIFT;
#endif
	return mod;
}

static int getKey(int keycode)
{
	keycode &= ~(Qt::MODIFIER_MASK | Qt::UNICODE_ACCEL);
	auto i = keyMapReverse.find(Qt::Key(keycode));
	if (i != keyMapReverse.end())
		return i->second;
	return 0;
}


void MainWindow::on_urlListWidget_doubleClicked(const QModelIndex &index)
{
	QString url;
	if (ui->urlListWidget->selectedItems().count() > 0)
	{
		url = ui->urlListWidget->selectedItems()[0]->data(Qt::UserRole).toString();
	}
	if (!url.isEmpty())
	{
		//postToThread([this, url]
		{
			boost::uuids::uuid id = boost::uuids::random_generator()();
			Json::Value root;
			root["id"] = boost::uuids::to_string(id);
			root["url"] = url.toStdString();
			root["title"] = url.toStdString();
			root["width"] = 300;
			root["height"] = 300;
			root["x"] = 0;
			root["y"] = 0;


			root["useResizeGrip"] = true;
			root["useDragFilter"] = true;
			root["useAppRegion"] = false;
			root["useDragMove"] = true;
			root["hide"] = false;
			root["zoom"] = 1.0;
			root["opacity"] = 1.0;
			root["fps"] = 30;
			root["Transparent"] = false;
			root["NoActivate"] = false;
			settingServer.set(root);
		};
	}
}

void MainWindow::on_newURL_returnPressed()
{
    QString url;
    {
        url = ui->newURL->text().trimmed();
        ui->newURL->setText("");
    }
    if(!url.isEmpty())
    {
        //postToThread([this, url]
        {
            boost::uuids::uuid id = boost::uuids::random_generator()();
            Json::Value root;
            root["id"] = boost::uuids::to_string(id);
            root["url"] = url.toStdString();
            root["title"] = url.toStdString();
            root["width"] = 300;
            root["height"] = 300;
            root["x"] = 0;
            root["y"] = 0;


            root["useResizeGrip"] = true;
            root["useDragFilter"] = true;
			root["useAppRegion"] = false;
            root["useDragMove"] = true;
            root["hide"] = false;
            root["zoom"] = 1.0;
            root["opacity"] = 1.0;
            root["fps"] = 30;
            root["Transparent"] = false;
            root["NoActivate"] = false;
            settingServer.set(root);
        };
    }
}

void MainWindow::on_addButton_clicked()
{
	QString url;
	if (ui->urlListWidget->selectedItems().count() > 0)
	{
		url = ui->urlListWidget->selectedItems()[0]->data(Qt::UserRole).toString();
	}
	else
	{
		url = ui->newURL->text().trimmed();
		ui->newURL->setText("");
	}
	if(!url.isEmpty())
	{
		//postToThread([this, url]
		{
			boost::uuids::uuid id = boost::uuids::random_generator()();
			Json::Value root;
			root["id"] = boost::uuids::to_string(id);
			root["url"] = url.toStdString();
			root["title"] = url.toStdString();
			root["width"] = 300;
			root["height"] = 300;
			root["x"] = 0;
			root["y"] = 0;


			root["useResizeGrip"] = true;
			root["useDragFilter"] = true;
			root["useAppRegion"] = false;
			root["useDragMove"] = true;
			root["hide"] = false;
			root["zoom"] = 1.0;
			root["opacity"] = 1.0;
			root["fps"] = 30;
			root["Transparent"] = false;
			root["NoActivate"] = false;
			settingServer.set(root);
		};
    }
}

bool MainWindow::DragModifiers()
{
	bool b = true;
	auto a = QApplication::keyboardModifiers();
	if (ui->dragCtrl->isChecked())
	{
		b = b && a.testFlag(Qt::ControlModifier);
	}
	if (ui->dragAlt->isChecked())
	{
		b = b && a.testFlag(Qt::AltModifier);
	}
	if (ui->dragShift->isChecked())
	{
		b = b && a.testFlag(Qt::ShiftModifier);
	}
	return b;
}

void MainWindow::on_show_all_shortcut_editingFinished()
{
	QKeySequenceEdit* edit = ui->show_all_shortcut;
	int value = edit->keySequence()[0];
	QKeySequence shortcut(value);
	edit->setKeySequence(shortcut);
	const int id = 1;
#ifdef _WIN32
	UnregisterHotKey(NULL, id);
	if (RegisterHotKey(NULL, id, getModifier(shortcut[0]) | 0x4000, getKey(shortcut[0])))
	{
		qDebug() << "Hotkey '" << shortcut << "' registered, using MOD_NOREPEAT flag";
	}
#endif
	saveSettings();
}

void MainWindow::on_manager_shortcut_editingFinished()
{
	QKeySequenceEdit* edit = ui->manager_shortcut;
	int value = edit->keySequence()[0];
	QKeySequence shortcut(value);
	edit->setKeySequence(shortcut);
	const int id = 2;
#ifdef _WIN32
	UnregisterHotKey(NULL, id);
	if (RegisterHotKey(NULL, id, getModifier(shortcut[0]) | 0x4000, getKey(shortcut[0])))
	{
		qDebug() << "Hotkey '" << shortcut << "' registered, using MOD_NOREPEAT flag";
	}
#endif
	saveSettings();
}

void MainWindow::on_click_through_all_shortcut_editingFinished()
{
	QKeySequenceEdit* edit = ui->click_through_all_shortcut;
	int value = edit->keySequence()[0];
	QKeySequence shortcut(value);
	edit->setKeySequence(shortcut);
	const int id = 3;
#ifdef _WIN32
	UnregisterHotKey(NULL, id);
	if (RegisterHotKey(NULL, id, getModifier(shortcut[0]) | 0x4000, getKey(shortcut[0])))
	{
		qDebug() << "Hotkey '" << shortcut << "' registered, using MOD_NOREPEAT flag";
	}
#endif
	saveSettings();
}

void MainWindow::on_no_focus_all_shortcut_editingFinished()
{
	QKeySequenceEdit* edit = ui->no_focus_all_shortcut;
	int value = edit->keySequence()[0];
	QKeySequence shortcut(value);
	edit->setKeySequence(shortcut);
	const int id = 4;
#ifdef _WIN32
	UnregisterHotKey(NULL, id);
	if (RegisterHotKey(NULL, id, getModifier(shortcut[0]) | 0x4000, getKey(shortcut[0])))
	{
		qDebug() << "Hotkey '" << shortcut << "' registered, using MOD_NOREPEAT flag";
	}
#endif
	saveSettings();
}

void MainWindow::on_draggable_all_shortcut_editingFinished()
{
	QKeySequenceEdit* edit = ui->draggable_all_shortcut;
	int value = edit->keySequence()[0];
	QKeySequence shortcut(value);
	edit->setKeySequence(shortcut);
	const int id = 5;
#ifdef _WIN32
	UnregisterHotKey(NULL, id);
	if (RegisterHotKey(NULL, id, getModifier(shortcut[0]) | 0x4000, getKey(shortcut[0])))
	{
		qDebug() << "Hotkey '" << shortcut << "' registered, using MOD_NOREPEAT flag";
	}
#endif
	saveSettings();
}

void MainWindow::on_click_through_all_toggled(bool checked)
{
	std::lock_guard<std::recursive_mutex> l(settingServer.m);
	globalSettings.nextSettings.useTransparent = checked;
	globalSettings.CheckDirty();

	for (auto i = settingServer.settings.begin(); i != settingServer.settings.end(); ++i) {
		i->second.ApplySettings();
	}

	globalSettings.settings = globalSettings.nextSettings;
	globalSettings.CheckDirty();
	saveSettings();
}

void MainWindow::on_no_focus_all_toggled(bool checked)
{
	std::lock_guard<std::recursive_mutex> l(settingServer.m);
	globalSettings.nextSettings.useNoActivate = checked;
	globalSettings.CheckDirty();

	for (auto i = settingServer.settings.begin(); i != settingServer.settings.end(); ++i) {
		i->second.ApplySettings();
	}

	globalSettings.settings = globalSettings.nextSettings;
	globalSettings.CheckDirty();
	saveSettings();
}

void MainWindow::on_show_all_toggled(bool checked)
{
	std::lock_guard<std::recursive_mutex> l(settingServer.m);
	globalSettings.nextSettings.useHide = !checked;
	globalSettings.CheckDirty();

	for (auto i = settingServer.settings.begin(); i != settingServer.settings.end(); ++i) {
		i->second.ApplySettings();
	}

	globalSettings.settings = globalSettings.nextSettings;
	globalSettings.CheckDirty();
	saveSettings();
}

void MainWindow::on_resizeable_all_toggled(bool checked)
{
	std::lock_guard<std::recursive_mutex> l(settingServer.m);
	globalSettings.nextSettings.useResizeGrip = checked;
	globalSettings.CheckDirty();

	for (auto i = settingServer.settings.begin(); i != settingServer.settings.end(); ++i) {
		i->second.ApplySettings();
	}

	globalSettings.settings = globalSettings.nextSettings;
	globalSettings.CheckDirty();
	saveSettings();
}

void MainWindow::on_draggable_all_toggled(bool checked)
{
	std::lock_guard<std::recursive_mutex> l(settingServer.m);
	globalSettings.nextSettings.useDragFilter = checked;
	globalSettings.nextSettings.useDragMove = checked;
	globalSettings.CheckDirty();

	for (auto i = settingServer.settings.begin(); i != settingServer.settings.end(); ++i) {
		i->second.ApplySettings();
	}

	globalSettings.settings = globalSettings.nextSettings;
	globalSettings.CheckDirty();
	saveSettings();
}

void MainWindow::on_dragCtrl_toggled(bool checked)
{
	saveSettings();
}

void MainWindow::on_dragAlt_toggled(bool checked)
{
	saveSettings();
}

void MainWindow::on_dragShift_toggled(bool checked)
{
	saveSettings();
}

void MainWindow::on_auto_hide_clicked()
{
	saveSettings();
}


void MainWindow::on_hide_filter_condition_currentIndexChanged(int index)
{
    saveSettings();
}

#include <boost/filesystem.hpp>
#include <fstream>
#include <json/json.h>

static boost::filesystem::path setting = "overlay_proc.json";

void MainWindow::saveSettings()
{
	if (setOnlyGlobal)
		return;
	Json::Value value;
	value["backgroundColor"] = ui->colorEdit->text().toStdString();
	value["backgroundStyle"] = ui->backgroundTextEdit->toPlainText().toStdString();

	value["backgroundImageUrl"] = ui->image_url_edit->text().toStdString();
	value["backgroundImageRepeat"] = ui->image_repeat_combo->currentText().toStdString();
	value["backgroundImagePosition"] = ui->image_position_combo->currentText().toStdString();
	value["backgroundImageSize"] = ui->image_size_combo->currentText().toStdString();

	//.arg(ui->image_url_edit->text())
	//	.arg(ui->image_repeat_combo->currentText())
	//	.arg(ui->image_position_combo->currentText())
	//	.arg(ui->image_size_combo->currentText());

	value["dragCtrl"] = ui->dragCtrl->isChecked();
	value["dragAlt"] = ui->dragAlt->isChecked();
	value["dragShift"] = ui->dragShift->isChecked();

	value["click_through_all"] = ui->click_through_all->isChecked();
	value["no_focus_all"] = ui->no_focus_all->isChecked();
	value["draggable_all"] = ui->draggable_all->isChecked();
	value["show_all"] = ui->show_all->isChecked();
	value["resizeable_all"] = ui->resizeable_all->isChecked();

	value["auto_hide"] = ui->auto_hide->isChecked();

	Json::Value processes;
	for (int i = 0; i < ui->processesListWidget->count(); ++i)
	{
		QListWidgetItem* item = ui->processesListWidget->item(i);
		std::string process = item->text().toLower().toUtf8().toStdString();
		processes.append(process);
	}

	value["process_list"] = processes;

	value["manager_short_cut"] = ui->manager_shortcut->keySequence().toString().toStdString();
	value["click_through_shortcut"] = ui->click_through_all_shortcut->keySequence().toString().toStdString();
	value["no_focus_shortcut"] = ui->no_focus_all_shortcut->keySequence().toString().toStdString();
	value["draggable_shortcut"] = ui->draggable_all_shortcut->keySequence().toString().toStdString();
	value["show_shortcut"] = ui->show_all_shortcut->keySequence().toString().toStdString();

	value["hide_filter_condition"] = ui->hide_filter_condition->currentIndex();
	value["renderer"] = ui->renderer->currentIndex();

	try
	{
#ifdef _WIN32
		std::ofstream fout(setting.wstring());
#else
		std::ofstream fout(setting.string().c_str());
#endif
		Json::StyledStreamWriter writer;
		writer.write(fout, value);
		fout.close();
	}
	catch (std::exception& e)
	{
	}

}
void MainWindow::loadSettings()
{
	setOnlyGlobal = true;
	if (boost::filesystem::exists(setting))
	{
		Json::Reader reader;
		Json::Value value;
		Json::Value zero(0);
#ifdef _WIN32
		std::ifstream fin(setting.wstring());
#else
		std::ifstream fin(setting.string().c_str());
#endif
		if (reader.parse(fin, value))
		{
			ui->colorEdit->setText(QString::fromStdString(value.get("backgroundColor", "#00000000").asString()));
			ui->backgroundTextEdit->setPlainText(QString::fromStdString(value.get("backgroundStyle", "").asString()));
			ui->image_url_edit->setText(QString::fromStdString(value.get("backgroundImageUrl", "").asString()));
			ui->image_repeat_combo->setCurrentText(QString::fromStdString(value.get("backgroundImageRepeat", "repeat").asString()));
			ui->image_position_combo->setCurrentText(QString::fromStdString(value.get("backgroundImagePosition", "top left").asString()));
			ui->image_size_combo->setCurrentText(QString::fromStdString(value.get("backgroundImageSize", "cover").asString()));

			globalBackground = ui->backgroundTextEdit->toPlainText();
			on_styleApplyButton_clicked();
			//globalBackground.setNamedColor(ui->colorEdit->text());

			ui->dragCtrl->setChecked(value.get("dragCtrl", false).asBool());
			ui->dragAlt->setChecked(value.get("dragAlt", false).asBool());
			ui->dragShift->setChecked(value.get("dragShift", false).asBool());

			ui->click_through_all->setChecked(value.get("click_through_all", false).asBool());
			ui->no_focus_all->setChecked(value.get("no_focus_all", false).asBool());
			ui->draggable_all->setChecked(value.get("draggable_all", true).asBool());
			ui->show_all->setChecked(value.get("show_all", true).asBool());
			ui->resizeable_all->setChecked(value.get("resizeable_all", true).asBool());

			ui->auto_hide->setChecked(value.get("auto_hide", true).asBool());

			ui->manager_shortcut->setKeySequence(QKeySequence::fromString(QString::fromStdString(value["manager_short_cut"].asString())));
			ui->click_through_all_shortcut->setKeySequence(QKeySequence::fromString(QString::fromStdString(value["click_through_shortcut"].asString())));
			ui->no_focus_all_shortcut->setKeySequence(QKeySequence::fromString(QString::fromStdString(value["no_focus_shortcut"].asString())));
			ui->draggable_all_shortcut->setKeySequence(QKeySequence::fromString(QString::fromStdString(value["draggable_shortcut"].asString())));
			ui->show_all_shortcut->setKeySequence(QKeySequence::fromString(QString::fromStdString(value["show_shortcut"].asString())));

			ui->hide_filter_condition->setCurrentIndex(value.get("hide_filter_condition", zero).asInt());
			ui->renderer->setCurrentIndex(value.get("renderer", zero).asInt());
			ui->processesListWidget->clear();
			Json::Value& processes = value["process_list"];
			for (int i = 0; i < processes.size(); ++i)
			{
				QByteArray ba = QByteArray::fromStdString(processes[i].asString());
				QListWidgetItem* item = new QListWidgetItem(QString::fromUtf8(ba), ui->processesListWidget);
			}
		}
		fin.close();
	}
	setOnlyGlobal = false;
}

void MainWindow::on_manager_shortcut_keySequenceChanged(const QKeySequence &keySequence)
{
    on_manager_shortcut_editingFinished();
}

void MainWindow::on_show_all_shortcut_keySequenceChanged(const QKeySequence &keySequence)
{
    on_show_all_shortcut_editingFinished();
}

void MainWindow::on_click_through_all_shortcut_keySequenceChanged(const QKeySequence &keySequence)
{
    on_click_through_all_shortcut_editingFinished();
}

void MainWindow::on_no_focus_all_shortcut_keySequenceChanged(const QKeySequence &keySequence)
{
    on_no_focus_all_shortcut_editingFinished();
}

void MainWindow::on_draggable_all_shortcut_keySequenceChanged(const QKeySequence &keySequence)
{
    on_draggable_all_shortcut_editingFinished();
}

void MainWindow::on_processAddButton_clicked()
{
	if (ui->processName->toPlainText().trimmed().size() > 0)
	{
		QListWidgetItem* item = new QListWidgetItem(ui->processName->toPlainText(), ui->processesListWidget);
		ui->processName->clear();
		saveSettings();
	}
}

void MainWindow::on_processRemoveButton_clicked()
{
	if (ui->processesListWidget->selectedItems().size() > 0)
	{
		auto items = ui->processesListWidget->selectedItems();
		for (auto i = items.begin(); i != items.end(); ++i)
		{
			delete *i;
		}
	}
	saveSettings();
}

void MainWindow::on_colorEdit_textChanged(const QString &arg1)
{
	QColor c;
	std::string zz = arg1.toStdString();
	c.setNamedColor(arg1);
	globalBackground.sprintf("document.body.style.background = \"rgba(%d, %d, %d, %f)\";",
		(int)(c.red()),
		(int)(c.green()),
		(int)(c.blue()),
		c.alphaF()
	);
	ui->backgroundTextEdit->setPlainText(globalBackground);
	{
		std::lock_guard<std::recursive_mutex> l(settingServer.m);

		for (auto i = settingServer.widgets.begin(); i != settingServer.widgets.end(); ++i)
		{
			OverlayMainWindow* mainWindow = i->second;
			if (mainWindow && mainWindow->view && mainWindow->view->page())
			{
				mainWindow->view->page()->runJavaScript(globalBackground);
			}
		}
	}
	saveSettings();
}

//void MainWindow::on_backgroundResetButton_clicked()
//{
//	ui->colorEdit->setText("#00000000");
//}

void MainWindow::on_backgroundTransparentButton_clicked()
{
	ui->colorEdit->setText("#00000000");
}

void MainWindow::on_backgroundWhiteButton_clicked()
{
	ui->colorEdit->setText("#ffffffff");
}

void MainWindow::on_renderer_currentIndexChanged(int index)
{
	saveSettings();
}

void MainWindow::showManager()
{
	this->setFocus();
	this->activateWindow();
	this->showNormal();
}
void MainWindow::exitManager()
{
	{

		Json::Value root;
		Json::Value null;
		Json::Value cmd;
		Json::Value ret;
		Json::FastWriter writer;
		try {
			ret["cmd"] = "overlay_proc_status_changed";
			std::string content = "";
			content = writer.write(ret);
			// hang
			Broadcast(0, QString::fromStdString(content));
		}
		catch (...)
		{
		}
	}
	saveSettings();
	delete this;
	QApplication::instance()->exit(0);
}
void MainWindow::slotActivated(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason)
	{
	case QSystemTrayIcon::Trigger:
	case QSystemTrayIcon::DoubleClick:
		this->show();
		break;
	default:
		break;
	}

}

void MainWindow::on_positionResetButton_clicked()
{
	QListWidgetItem* currentItem = ui->listWidget->currentItem();
	if (currentItem != nullptr)
	{
		QVariant data = currentItem->data(Qt::UserRole);
		QString idStr = data.toString();
		auto id = boost::lexical_cast<boost::uuids::uuid>(idStr.toStdString());
		settingServer.m.lock();
		auto i = settingServer.settings.find(id);
		if (i != settingServer.settings.end())
		{
			setOnly = true;
			i->second.settingsDirty = true;
			i->second.nextSettings.x = 0;
			i->second.nextSettings.y = 0;
			i->second.nextSettings.width = 300;
			i->second.nextSettings.height = 300;
			ResetPosition(&i->second.nextSettings, id);
			i->second.ApplySettings(false, true);
			setOnly = false;
		}
		settingServer.m.unlock();
	}
}

void MainWindow::on_colorPickerButton_clicked()
{
	QColor c;
	c.setNamedColor(ui->colorEdit->text());
	QColorDialog::ColorDialogOptions options;
	options.setFlag(QColorDialog::ShowAlphaChannel, true);

	QColorDialog dlg(this);
	dlg.setWindowTitle(tr("Select Background Color"));
	dlg.setOptions(options);
	dlg.setCurrentColor(c);
	int ret = dlg.exec();
	if (ret == 1)
	{
		c = dlg.selectedColor();
		QString name;
		name.sprintf("#%02X%02X%02X%02X", c.alpha(), c.red(), c.green(), c.blue());
		ui->colorEdit->setText(name);
	}
}

void MainWindow::on_styleApplyButton_clicked()
{
	globalBackground = ui->backgroundTextEdit->toPlainText();
	{
		std::lock_guard<std::recursive_mutex> l(settingServer.m);

		for (auto i = settingServer.widgets.begin(); i != settingServer.widgets.end(); ++i)
		{
			OverlayMainWindow* mainWindow = i->second;
			if (mainWindow && mainWindow->view && mainWindow->view->page())
			{
				mainWindow->view->page()->runJavaScript(globalBackground);
			}
		}
	}
	saveSettings();
}

void MainWindow::on_backgroundTabWidget_currentChanged(int index)
{

}

void MainWindow::on_apply_image_button_clicked()
{
	//object.style.background = color image repeat attachment position size origin clip | initial | inherit
	globalBackground = QString("document.body.style.background = \"url(%1) %2 %3\"; document.body.style.backgroundSize = \"%4\"")
		.arg(ui->image_url_edit->text())
		.arg(ui->image_repeat_combo->currentText())
		.arg(ui->image_position_combo->currentText())
		.arg(ui->image_size_combo->currentText());
	std::string ss = globalBackground.toStdString();
	ui->backgroundTextEdit->setPlainText(globalBackground);
	{
		std::lock_guard<std::recursive_mutex> l(settingServer.m);

		for (auto i = settingServer.widgets.begin(); i != settingServer.widgets.end(); ++i)
		{
			OverlayMainWindow* mainWindow = i->second;
			if (mainWindow && mainWindow->view && mainWindow->view->page())
			{
				mainWindow->view->page()->runJavaScript(globalBackground);
			}
		}
	}
	saveSettings();

}
