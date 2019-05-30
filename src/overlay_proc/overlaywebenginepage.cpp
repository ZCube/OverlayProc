#include "overlaywebenginepage.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QWebEngineFullScreenRequest>
#include <QIcon>
#include <QApplication>

void OverlayWebEnginePage::fullScreenRequested(QWebEngineFullScreenRequest fullScreenRequest)
{
	QWebEnginePage::fullScreenRequested(fullScreenRequest);
}

void OverlayWebEnginePage::javaScriptAlert(const QUrl &securityOrigin, const QString &msg)
{
	Q_UNUSED(securityOrigin);
	QMessageBox::information(view(), QStringLiteral("Javascript Alert - %1").arg(url().toString()), msg);
}

bool OverlayWebEnginePage::javaScriptConfirm(const QUrl &securityOrigin, const QString &msg)
{
	Q_UNUSED(securityOrigin);
	return (QMessageBox::information(view(), QStringLiteral("Javascript Confirm - %1").arg(url().toString()), msg, QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok);
}

bool OverlayWebEnginePage::javaScriptPrompt(const QUrl &securityOrigin, const QString &msg, const QString &defaultValue, QString *result)
{
	Q_UNUSED(securityOrigin);
	bool ret = false;
	if (result)
		*result = QInputDialog::getText(view(), QStringLiteral("Javascript Prompt - %1").arg(url().toString()), msg, QLineEdit::Normal, defaultValue, &ret);
	return ret;
}

void OverlayWebEnginePage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceID)
{
	Q_UNUSED(level);
	Q_UNUSED(message);
	Q_UNUSED(lineNumber);
	Q_UNUSED(sourceID);
}

bool OverlayWebEnginePage::certificateError(const QWebEngineCertificateError &)
{
	return true;
}

bool OverlayWebEnginePage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame)
{
	Q_UNUSED(url);
	Q_UNUSED(type);
	Q_UNUSED(isMainFrame);
	return true;
}

QWebEnginePage *OverlayWebEnginePage::createWindow(WebWindowType type)
{
	//OverlayMainWindow* main = new OverlayMainWindow();
	QWebEngineView* m = new OverlayWebEngineView();
	m->setWindowIcon(QIcon(":/images/icon.png"));
	QWebEnginePage* p = new OverlayWebEnginePage(m);
	m->setPage(p);
	//main->show();
	m->setWindowTitle(m->title());
	m->show();
	QObject::connect(m, &QWebEngineView::titleChanged, QApplication::instance(), std::move([m, this] {
		m->setWindowTitle(m->title());
	}));
	//main->setWindowTItle
	return p;
}
