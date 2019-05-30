#include "utility.h"
#include <QWindow>

void SetOpacity(QWidget* w, qreal opacity) {
	w->setWindowOpacity(opacity);

	const QObjectList& q = w->children();
	for (int i = 0; i < q.size(); ++i) {
		QObject* p = q.at(i);
		if (p->isWidgetType()) {
			SetOpacity((QWidget*)p, opacity);
		}
	}
}

void SetWindowBlur(HWND hWnd)
{
#ifdef _WIN32

	const HINSTANCE hModule = LoadLibrary(TEXT("user32.dll"));
	if (hModule)
	{
		struct ACCENTPOLICY
		{
			int nAccentState;
			int nFlags;
			int nColor;
			int nAnimationId;
		};
		struct WINCOMPATTRDATA
		{
			int nAttribute;
			PVOID pData;
			ULONG ulDataSize;
		};
		typedef BOOL(WINAPI*pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
		const pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(hModule, "SetWindowCompositionAttribute");
		if (SetWindowCompositionAttribute)
		{
			ACCENTPOLICY policy = { 3, 0, 0, 0 };
			WINCOMPATTRDATA data = { 19, &policy, sizeof(ACCENTPOLICY) };
			SetWindowCompositionAttribute(hWnd, &data);
		}
		FreeLibrary(hModule);
	}
#endif
}
//HWND hwnd = (HWND)this->effectiveWinId();
//SetWindowBlur(hwnd);

void SetTransparent(QWidget* w, bool t) {
	w->setAttribute(Qt::WA_TranslucentBackground, true);
	w->setAttribute(Qt::WA_TransparentForMouseEvents, t);


	const QObjectList& q = w->children();
	for (int i = 0; i < q.size(); ++i) {
		QObject* p = q.at(i);
		if (p->isWidgetType()) {
			SetTransparent((QWidget*)p, t);
			//((QWidget*)p)->setMouseTracking(true);

		}
	}
	//QPixmap pixmap = QWidget::grab( this->rect() );
	//this->setMask( pixmap.createMaskFromColor( Qt::transparent, Qt::MaskInColor ) );
}

void SetMouseTrackable(QWidget* w, bool o) {
	w->setMouseTracking(o);
	const QObjectList& q = w->children();
	for (int i = 0; i < q.size(); ++i) {
		QObject* p = q.at(i);
		if (p->isWidgetType()) {
			SetMouseTrackable((QWidget*)p, o);
		}
	}
}

void InstallEvent(QWidget* w, QObject* o) {
	w->installEventFilter(o);
	const QObjectList& q = w->children();
	for (int i = 0; i < q.size(); ++i) {
		QObject* p = q.at(i);
		if (p->isWidgetType()) {
			InstallEvent((QWidget*)p, o);
		}
	}
}

static QWindow* windowForWidget(const QWidget* widget)
{
	QWindow* window = widget->windowHandle();
	if (window)
		return window;
	const QWidget* nativeParent = widget->nativeParentWidget();
	if (nativeParent)
		return nativeParent->windowHandle();
	return 0;
}

HWND getHWNDForWidget(const QWidget* widget)
{
	QWindow* window = ::windowForWidget(widget);
	if (window && window->handle())
	{
		return (HWND)(window->winId());
	}
	return 0;
}
