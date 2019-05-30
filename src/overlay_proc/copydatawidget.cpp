#include "copydatawidget.h"
#ifdef _WIN32
#include <Windows.h>
#endif
#include <boost/algorithm/string.hpp>
#include <iostream>

#ifdef _WIN32
BOOL CALLBACK EnumWindowCallBack(HWND hwnd, LPARAM lParam)
{
	// className Check First
	wchar_t className[1024] = { 0, };
	int ret = ::GetClassNameW(hwnd, className, 1023);
	if (ret == 0)
		return TRUE;

	DWORD lpdwProcessId;
	DWORD currentid;
	GetWindowThreadProcessId(hwnd, &lpdwProcessId);
	currentid = GetCurrentProcessId();
	if (currentid == lpdwProcessId)
		return TRUE;

	// windowText Length Check
	int length = ::GetWindowTextLength(hwnd);
	if (0 == length) return TRUE;

	// getWindowName and append it
	std::list<HWND>* list = (std::list<HWND>*)lParam;
	wchar_t title[1024] = { 0, };
	ret = GetWindowTextW(hwnd, title, 1024);
	if (ret == 0)
		return TRUE;

	if (boost::starts_with(title, L"ClientOverlayProcWMCOPYDATA"))
	{
		list->push_back(hwnd);
	}
	return TRUE;
}

#endif
// CopyDataWidget

CopyDataWidget::CopyDataWidget(MsgProc* p, QWidget* parent)
	: QWidget(parent), proc(p)
{

}
bool CopyDataWidget::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
#ifdef _WIN32
	Q_UNUSED(result);
	Q_UNUSED(eventType);
	MSG *param = static_cast<MSG *>(message);

	switch (param->message)
	{
	case WM_COPYDATA:
	{
		COPYDATASTRUCT *cds = reinterpret_cast<COPYDATASTRUCT*>(param->lParam);
		// 0end string
		param->wParam;
		QString strMessage = QString::fromUtf8(reinterpret_cast<char*>(cds->lpData), cds->cbData);
		if (proc)
		{
			proc->HandleData(ClientID((HWND)param->wParam), cds->dwData, strMessage);
		}

		return true;
	}
	break;
	}
#endif
	return QWidget::nativeEvent(eventType, message, result);
}
void CopyDataWidget::SendTo(ClientID sender, int code, const QString & data)
{
#ifdef _WIN32
	{
		QByteArray ba = data.toUtf8();
		ba.append("\0");
		COPYDATASTRUCT copydata;
		copydata.dwData = code;
		copydata.lpData = ba.data();
		copydata.cbData = ba.size();

		{
			if (::IsWindow(sender.hwnd))
			{
				HWND from = (HWND)this->effectiveWinId();
				::SendMessage(sender.hwnd, WM_COPYDATA, reinterpret_cast<WPARAM>(from), reinterpret_cast<LPARAM>(&copydata));
			}
		}
	}
#endif
}
void CopyDataWidget::Broadcast(int code, const QString & data)
{
#ifdef _WIN32
	std::list<HWND> hwndList;
	BOOL b = EnumWindows(::EnumWindowCallBack, (LPARAM)&hwndList);
	if (b)
	{
		QByteArray ba = data.toUtf8();
		ba.append("\0");
		COPYDATASTRUCT copydata;
		copydata.dwData = code;
		copydata.lpData = ba.data();
		copydata.cbData = ba.size();

		HWND sender = (HWND)this->effectiveWinId();
		if (::IsWindow(sender))
		{
			for (auto hwnd : hwndList)
			{
				if (::IsWindow(hwnd))
				{
					wchar_t title[255] = { 0, };
					GetWindowTextW(hwnd, title, 255);
					if (boost::starts_with(title, L"ClientOverlayProcWMCOPYDATA"))
					{
						::SendMessage(hwnd, WM_COPYDATA, reinterpret_cast<WPARAM>(sender), reinterpret_cast<LPARAM>(&copydata));
					}
				}
			}
		}
	}
#endif
}
