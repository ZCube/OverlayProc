#include "../version.h"
#include <QApplication>
#include <QtWebEngineWidgets/QtWebEngineWidgets>
#include <iostream>
#include <QMainWindow>
#include <QWidget>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <json/json.h>
#include <settings/settings.h>
#include <QSystemTrayIcon>
#include <QMenu>
#include "main.h"
#include "mainwindow.h"
#include <QtGlobal>
#include <QStandardPaths>

QApplication* app = nullptr;
MainWindow* manager = nullptr;

static boost::filesystem::path setting = "overlay_proc.json";
std::string QtWebEngineVersion;
std::string ChromeVersion;

int main(int argc, char *argv[])
{
	int renderer = 0;
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
			renderer = value.get("renderer", zero).asInt();
		}
		fin.close();
	}
	switch (renderer)
	{
	case 0:
		QApplication::setAttribute(Qt::AA_UseDesktopOpenGL, true);
		break;
	case 1:
		QApplication::setAttribute(Qt::AA_UseOpenGLES, true);
		break;
	case 2:
		QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL, true);
		break;
	}

#ifdef _DEBUG
	QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL, true);
#endif

	// chrome arguments
	std::vector<const char*> args;
	for (int i = 0; i < argc; ++i)
	{
		if(argv[i])
			args.push_back(argv[i]);
	}
	args.push_back("--allow-running-insecure-content");
	uint16_t server_port = 9991;
	uint16_t debug_port = 9993;

	// port setting
	if (debug_port > 0) {
		qputenv("QTWEBENGINE_REMOTE_DEBUGGING", QString::number(debug_port).toUtf8());
	}

	argc = args.size();
	args.push_back(nullptr);

	QApplication a(argc, (char**)args.data());

	{
		wchar_t szPath[1024] = { 0 };
		GetModuleFileNameW(NULL, szPath, 1023);
		boost::filesystem::path p(szPath);
		p = p.parent_path();
		//std::wstring f = p.filename().wstring();
		boost::filesystem::create_directories(p / "Cache");
		boost::filesystem::create_directories(p / "LocalStorage");
		QString cachePath = QString::fromStdWString((p / "Cache").wstring());
		QString persistentStoragePath = QString::fromStdWString((p / "LocalStorage").wstring());
		auto * profile = QWebEngineProfile::defaultProfile();
		QWebEngineProfile::defaultProfile()->setCachePath(cachePath);
		QWebEngineProfile::defaultProfile()->setPersistentStoragePath(persistentStoragePath);
	}
	QString useragent = QWebEngineProfile::defaultProfile()->httpUserAgent();
	//QString useragent = QWebEngineProfile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::WebAttribute::)
	std::string ver = useragent.toStdString();
	//"Mozilla/5.0 (Windows NT 6.2; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) QtWebEngine/5.8.0 Chrome/53.0.2785.148 Safari/537.36";

	std::vector<std::string> strs;
	boost::split(strs, ver, boost::is_any_of(" "));
	for (auto str : strs)
	{
		if (boost::starts_with(str, "QtWebEngine"))
		{
			QtWebEngineVersion = str;
			boost::algorithm::replace_all(QtWebEngineVersion, "/", " ");
		}
		if (boost::starts_with(str, "Chrome/"))
		{
			ChromeVersion = str;
			boost::algorithm::replace_all(ChromeVersion, "/", " ");
		}
	}

	// init locale
	QLocale::setDefault(QLocale::system());
	QTranslator* translator = new QTranslator(&a);
	if (translator->load("overlay_proc_" + QLocale::system().bcp47Name() + ".qm"))
	{
		QApplication::installTranslator(translator);
	}
	else if (translator->load("translations/overlay_proc_" + QLocale::system().bcp47Name() + ".qm"))
	{
		QApplication::installTranslator(translator);
	}

	// application
	QCoreApplication::setOrganizationDomain("ZCube.kr");
	QCoreApplication::setOrganizationName("ZCube");
	QApplication::setApplicationName("OverlayProc");
	QApplication::setApplicationVersion(version_string);

	// main window
	manager = new MainWindow(server_port);
	manager->show();
	manager->setWindowTitle(manager->windowTitle() + " " + version_string);
	manager->setWindowIcon(QIcon(":/images/icon.png"));

	// webengine cache delete
	{
		QString loc = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
		QString dataPath = loc + "/Profiles/" + QApplication::applicationName() + "/QtWebEngine/Default";
		std::string s = loc.toStdString();
		QDir(dataPath).remove("Visited Links");
	}

#ifdef WIN32
	while (true)
	{
		HMODULE hModule;
		void(__stdcall *funcp) (LPCTSTR);

		try {
			hModule = LoadLibraryA("dwmapi.dll");
			if (!hModule)
				throw std::exception("");

			int DWM_EC_DISABLECOMPOSITION = 0;
			int DWM_EC_ENABLECOMPOSITION = 1;

			HRESULT(WINAPI *DwmEnableComposition)			(UINT uCompositionAction);
			HRESULT(WINAPI *DwmIsCompositionEnabled)		(BOOL*);

			HRESULT ret = S_OK;
			DwmEnableComposition = (HRESULT(WINAPI *)(UINT)) GetProcAddress(hModule, "DwmEnableComposition");
			DwmIsCompositionEnabled = (HRESULT(WINAPI *)(BOOL*))GetProcAddress(hModule, "DwmIsCompositionEnabled");

			if (!DwmEnableComposition)
				throw std::exception("");
			if (!DwmIsCompositionEnabled)
				throw std::exception("");

			BOOL com = false;

			ret = DwmIsCompositionEnabled(&com);

			if (!com || ret != S_OK)
			{
				ret = DwmEnableComposition(DWM_EC_ENABLECOMPOSITION);
				ret = DwmIsCompositionEnabled(&com);
				if (!com || ret != S_OK)
				{
					throw std::exception("");
				}
			}
		}
		catch (std::exception e)
		{
			QMessageBox msgBox;
			msgBox.setWindowTitle(QObject::tr("Compatibility problem occured"));
			msgBox.setInformativeText(QObject::tr("You need to enable the Windows Aero Glass feature. Use themes that support Aero Glass."));
			msgBox.setStandardButtons(QMessageBox::Ok);
			msgBox.setDefaultButton(QMessageBox::Ok);
			msgBox.exec();
		}
		break;
	}
#endif

	// TODO: move to mainwindow
	// load
	boost::filesystem::path p = "overlays.json";

	if (boost::filesystem::exists(p))
	{
		Json::Reader reader;
		Json::Value val;
#ifdef _WIN32
		std::ifstream fin(p.wstring());
#else
		std::ifstream fin(p.string().c_str());
#endif
		if (reader.parse(fin, val))
		{
			settingServer.set_all(val);
		}
		fin.close();
	}

	// run
	int ret = a.exec();
	
	// exit
	//delete manager;
	try {
	}
	catch (...)
	{

	}
	return ret;
}
