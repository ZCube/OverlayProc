#pragma once

#include <QWebSocketServer>
#include <QWebSocket>
#include "utility.h"

QT_FORWARD_DECLARE_CLASS(QWebSocketServer)
QT_FORWARD_DECLARE_CLASS(QWebSocket)

class WebSocketServer : public QObject, public Sendable
{
	Q_OBJECT
	MsgProc* msgProc;
public:
	explicit WebSocketServer(MsgProc* msgProc_, QHostAddress address, quint16 port, QObject *parent = Q_NULLPTR);
	virtual ~WebSocketServer();
	QList<QWebSocket *> m_clients;

	virtual void SendTo(ClientID sender, int code, const QString & data);
	virtual void Broadcast(int code, const QString & data);
signals:
	void HandleData(ClientID sender, int code, const QString & data);
Q_SIGNALS:
	void closed();

	private Q_SLOTS:
	void onNewConnection();
	void processTextMessage(QString message);
	void processBinaryMessage(QByteArray message);
	void socketDisconnected();

private:
	QWebSocketServer *m_pWebSocketServer;
	bool m_debug;
};
