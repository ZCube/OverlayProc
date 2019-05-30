#pragma once
#include <string>
#include <stdint.h>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <set>

#include <boost/asio.hpp>
#include <functional>
#include <string>
#include <iostream>


class OverlaySettings
{
public:
	bool useDragMove = true;
	std::string title = "";
	std::string url = "";
	int32_t width = 512;
	int32_t height = 512;
	int32_t x = 0;
	int32_t y = 0;
	double zoom = 0.5;
	double opacity = 0.5;
	bool useHide = false;
	double fps = 30.0;

	// force for overlay screen
	bool useDragFilter = true;
	bool useAppRegion = false;
	// windows only
	bool useResizeGrip = true;
	bool useTransparent = false;
	bool useNoActivate = false;
};

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <unordered_map>

class OverlaySettingsWithDirtyFlag
{
public:
	OverlaySettingsWithDirtyFlag()
	{
		settingsDirty = false;
	}
	~OverlaySettingsWithDirtyFlag()
	{
	}
	void CheckDirty();
	OverlaySettings settings;
	OverlaySettings nextSettings;
	bool settingsDirty;
};

extern OverlaySettingsWithDirtyFlag globalSettings;

class OverlaySettingsManager : public OverlaySettingsWithDirtyFlag
{
	boost::uuids::uuid tag;
public:
	OverlaySettingsManager()
		: tag(boost::uuids::random_generator()()) {
		applySettingsNotRecursive = true;
	}
	OverlaySettingsManager(const boost::uuids::uuid& id)
		: tag(id) {
		applySettingsNotRecursive = true;
	}
	~OverlaySettingsManager()
	{
	}
	std::string id()
	{
		return boost::uuids::to_string(tag);
	}
	void ApplySettings(const Json::Value& value, bool applyAll = false);
	void ApplySettings(bool applyAll = false, bool withPosition = false);
	void UpdatePositions();
	void UpdateNextPositions();
	Json::Value GetPositions();
	bool UpdateSettings(const boost::uuids::uuid& id, const Json::Value& value, bool apply = false);
	Json::Value GetSettings(bool next = false);

	inline std::recursive_mutex& mutex() { return m; }
	std::recursive_mutex m;
	bool applySettingsNotRecursive;
};

class OverlaySettingsServer
{
public:
	OverlaySettingsServer() {
	}

	inline std::recursive_mutex& mutex() { return m; }
	std::recursive_mutex m;
	std::map<boost::uuids::uuid, OverlaySettingsManager> settings;

	void UpdateTitle(const boost::uuids::uuid& id, const std::string& title);
	bool UpdateOverlaySettings(const boost::uuids::uuid& id, const Json::Value& value, bool apply = false);

	virtual Json::Value NewOverlayWindow(const boost::uuids::uuid& id, const Json::Value& value) = 0;
	virtual void CloseOverlayWindow(const boost::uuids::uuid& id) = 0;
	virtual Json::Value CaptureOverlayWindow(const boost::uuids::uuid& id) = 0;

	Json::Value GetOverlaySettings(const boost::uuids::uuid& id, bool next = false);
	Json::Value GetOverlaySettings();
	Json::Value SetOverlaySettings(const Json::Value& value);

	Json::Value set(const Json::Value& value);
	Json::Value set_all(const Json::Value& value);
	Json::Value get(const Json::Value& value);
	Json::Value get_all();
	Json::Value close(const Json::Value& value);
	Json::Value close_all();
	Json::Value capture(const Json::Value& value);
};

//extern OverlaySettingsServer settingServer;
//extern OverlaySettingsManager settingManager;