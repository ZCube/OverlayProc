#include "websocketserver.h"
#include "utility.h"
#include <json/json.h>

WebSocketServer::WebSocketServer(MsgProc* msgProc_, QHostAddress address, quint16 port, QObject *parent) :
	QObject(parent), msgProc(msgProc_),
	m_pWebSocketServer(new QWebSocketServer(QStringLiteral("Overlay Manager Server"),
		QWebSocketServer::NonSecureMode, this)),
	m_clients()
{
	if (m_pWebSocketServer->listen(address, port)) {
		connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
			this, &WebSocketServer::onNewConnection);
		connect(m_pWebSocketServer, &QWebSocketServer::closed, this, &WebSocketServer::closed);
	}
}

WebSocketServer::~WebSocketServer()
{
	m_pWebSocketServer->close();
	m_clients.clear();
	//qDeleteAll(m_clients.begin(), m_clients.end());
}

void WebSocketServer::processBinaryMessage(QByteArray message)
{
	QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
	if (pClient) {
		pClient->sendBinaryMessage(message);
	}
}

void WebSocketServer::socketDisconnected()
{
	QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
	if (pClient) {
		m_clients.removeAll(pClient);
		pClient->deleteLater();
	}
}

void WebSocketServer::onNewConnection()
{
	QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();

	connect(pSocket, &QWebSocket::textMessageReceived, this, &WebSocketServer::processTextMessage);
	connect(pSocket, &QWebSocket::binaryMessageReceived, this, &WebSocketServer::processBinaryMessage);
	connect(pSocket, &QWebSocket::disconnected, this, &WebSocketServer::socketDisconnected);

	m_clients << pSocket;
}

void WebSocketServer::SendTo(ClientID sender, int code, const QString & data)
{
	sender.socket->sendTextMessage(data);
}

void WebSocketServer::Broadcast(int code, const QString & data)
{
	for (auto i = m_clients.begin(); i != m_clients.end(); ++i)
	{
		(*i)->sendTextMessage(data);
	}
}

void WebSocketServer::processTextMessage(QString message)
{
	QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
	if (msgProc)
	{
		msgProc->HandleData(ClientID(pClient), 0, message);
	}
	//emit HandleData(ClientID(pClient), 0, message);
}
