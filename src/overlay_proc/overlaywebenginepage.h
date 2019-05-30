#include <QWebEnginePage>
#include <QWebEngineView>
#include <QWidget>

class OverlayWebEngineView : public QWebEngineView
{
	Q_OBJECT
public:
	OverlayWebEngineView(QWidget* parent = nullptr)
		: QWebEngineView(parent)
	{
	}

	virtual ~OverlayWebEngineView()
	{
	}

	virtual void closeEvent(QCloseEvent * e)
	{
		QWebEngineView::closeEvent(e);
	}
};

class OverlayWebEnginePage : public QWebEnginePage
{
public:
	Q_OBJECT
public:
	OverlayWebEnginePage(QWidget* parent = nullptr)
		: QWebEnginePage(parent)
	{
	}
	virtual ~OverlayWebEnginePage()
	{
	}
	virtual void fullScreenRequested(QWebEngineFullScreenRequest fullScreenRequest);

	//virtual QStringList chooseFiles(FileSelectionMode mode, const QStringList &oldFiles, const QStringList &acceptedMimeTypes);
	virtual void javaScriptAlert(const QUrl &securityOrigin, const QString& msg);
	virtual bool javaScriptConfirm(const QUrl &securityOrigin, const QString& msg);
	virtual bool javaScriptPrompt(const QUrl &securityOrigin, const QString& msg, const QString& defaultValue, QString* result);
	virtual void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString& message, int lineNumber, const QString& sourceID);
	virtual bool certificateError(const QWebEngineCertificateError &certificateError);
	virtual bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame);
	QWebEnginePage *createWindow(WebWindowType type);
};
