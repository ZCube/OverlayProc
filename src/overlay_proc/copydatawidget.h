#pragma once
#include <QWidget>
#include "utility.h"

class CopyDataWidget : public QWidget, public Sendable
{
	Q_OBJECT
protected:
	MsgProc* proc;
public:
	CopyDataWidget(MsgProc* p = nullptr, QWidget* parent = nullptr);
	virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result);
	void SendTo(ClientID sender, int code, const QString & data);
	void Broadcast(int code, const QString & data);
};
