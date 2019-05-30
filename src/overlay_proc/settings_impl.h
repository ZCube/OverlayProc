#pragma once
#include "overlaymainwindow.h"

class QOverlaySettingsServer : public OverlaySettingsServer
{
public:
    virtual Json::Value NewOverlayWindow( const boost::uuids::uuid& id, const Json::Value& value );
    virtual void CloseOverlayWindow( const boost::uuids::uuid& id );
	virtual Json::Value CaptureOverlayWindow(const boost::uuids::uuid& id);
    std::map<boost::uuids::uuid, OverlayMainWindow*> widgets;
public slots:
	void loadStarted(OverlayMainWindow* m, QWebEngineView* v);
};
