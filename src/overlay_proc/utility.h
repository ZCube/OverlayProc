#pragma once

#include <QWidget>
#include <QWebSocket>
#ifdef _WIN32
#include <Windows.h>
#else
typedef void* HWND;
#endif
#include <sstream>

void SetOpacity(QWidget* w, qreal opacity);
void SetTransparent(QWidget* w, bool t = true);
void SetMouseTrackable(QWidget* w, bool o);
HWND getHWNDForWidget(const QWidget* widget);
void InstallEvent(QWidget* w, QObject* o);

class ClientID
{
public:
#ifdef _WIN32
	HWND hwnd;
#endif
	QWebSocket* socket;
	ClientID()
	{
#ifdef _WIN32
		hwnd = 0;
#endif
		socket = nullptr;
	}
#ifdef _WIN32
	ClientID(HWND h)
	{
		hwnd = h;
		socket = nullptr;
	}
#endif
	ClientID(QWebSocket* s)
	{
#ifdef _WIN32
		hwnd = 0;
#endif
		socket = s;
	}
	std::string Dump() const
	{
		std::stringstream sstr;
#ifdef _WIN32
		if (hwnd)
		{
			sstr << "HWND:" << hwnd;
		}
#endif
		if (socket)
		{
			sstr << "Socket:" << socket;
		}
		return sstr.str();
	}
};

class Sendable
{
public:
	virtual void SendTo(ClientID sender, int code, const QString & data) = 0;
	virtual void Broadcast(int code, const QString & data) = 0;
};

class MsgProc
{
public:
	virtual void HandleData(ClientID sender, int code, const QString & data) = 0;
};
