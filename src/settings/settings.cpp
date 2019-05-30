#include <json/json.h>
#include <thread>
#include <mutex>
#include <list>
#include "settings.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

OverlaySettingsWithDirtyFlag globalSettings;

//OverlaySettingsServer settingServer;
//OverlaySettingsManager settingManager;

bool OverlaySettingsManager::UpdateSettings(const boost::uuids::uuid& id, const Json::Value& root, bool apply)
{
	bool ret = false;
	{
		this->tag = id;
		std::lock_guard <std::recursive_mutex> w(m);
		UpdatePositions();
		nextSettings.useResizeGrip = root.get("useResizeGrip", settings.useResizeGrip).asBool();
		nextSettings.useDragFilter = root.get("useDragFilter", settings.useDragFilter).asBool();
		nextSettings.useAppRegion = root.get("useAppRegion", settings.useAppRegion).asBool();
		nextSettings.useDragMove = root.get("useDragMove", settings.useDragMove).asBool();
		nextSettings.title = root.get("title", settings.title).asString();
		nextSettings.url = root.get("url", settings.url).asString();
		nextSettings.width = root.get("width", settings.width).asInt();
		nextSettings.height = root.get("height", settings.height).asInt();
		nextSettings.x = root.get("x", settings.x).asInt();
		nextSettings.y = root.get("y", settings.y).asInt();
		nextSettings.useHide = root.get("hide", settings.useHide).asBool();
		nextSettings.zoom = root.get("zoom", settings.zoom).asDouble();
		nextSettings.opacity = root.get("opacity", settings.opacity).asDouble();
		nextSettings.fps = root.get("fps", settings.fps).asDouble();
		nextSettings.useTransparent = root.get("Transparent", settings.useTransparent).asBool();
		nextSettings.useNoActivate = root.get("NoActivate", settings.useNoActivate).asBool();
		CheckDirty();
		ret = true;
	}
	if (apply)
	{
		ApplySettings(root);
	}
	return ret;
}


void OverlaySettingsWithDirtyFlag::CheckDirty()
{
	settingsDirty =
		nextSettings.useResizeGrip != settings.useResizeGrip ||
		nextSettings.useDragFilter != settings.useDragFilter ||
		nextSettings.useAppRegion != settings.useAppRegion ||
		nextSettings.useDragMove != settings.useDragMove ||
		nextSettings.title != settings.title ||
		nextSettings.url != settings.url ||
		nextSettings.width != settings.width ||
		nextSettings.height != settings.height ||
		nextSettings.x != settings.x ||
		nextSettings.y != settings.y ||
		nextSettings.useHide != settings.useHide ||
		nextSettings.zoom != settings.zoom ||
		nextSettings.opacity != settings.opacity ||
		nextSettings.useTransparent != settings.useTransparent ||
		nextSettings.useNoActivate != settings.useNoActivate ||
		false
		;
}
Json::Value OverlaySettingsManager::GetSettings(bool next)
{
	OverlaySettings* s = nullptr;
	s = next ? &nextSettings : &settings;
	UpdatePositions();

	Json::Value root;
	{
		std::lock_guard<std::recursive_mutex> r(m);
		root["useResizeGrip"] = s->useResizeGrip;
		root["useDragFilter"] = s->useDragFilter;
		root["useAppRegion"] = s->useAppRegion;
		root["useDragMove"] = s->useDragMove;
		root["title"] = s->title;
		root["url"] = s->url;
		root["width"] = s->width;
		root["height"] = s->height;
		root["x"] = s->x;
		root["y"] = s->y;
		root["hide"] = s->useHide;
		root["zoom"] = s->zoom;
		root["opacity"] = s->opacity;
		root["fps"] = s->fps;
		root["Transparent"] = s->useTransparent;
		root["NoActivate"] = s->useNoActivate;
	}
	return root;
}

Json::Value OverlaySettingsManager::GetPositions()
{
	OverlaySettings* s = &settings;
	Json::Value root;
	{
		root["width"] = s->width;
		root["height"] = s->height;
		root["x"] = s->x;
		root["y"] = s->y;
	}
	return root;
}

void OverlaySettingsServer::UpdateTitle(const boost::uuids::uuid& id, const std::string& title)
{
	std::lock_guard<std::recursive_mutex> l(m);
	Json::Value value;
	{
		value["id"] = boost::uuids::to_string(id);
		value["title"] = title;
	}
	settings[id].UpdateSettings(id, value, true);
}

bool OverlaySettingsServer::UpdateOverlaySettings(const boost::uuids::uuid& id, const Json::Value& value, bool apply) {
	std::lock_guard<std::recursive_mutex> l(m);
	return settings[id].UpdateSettings(id, value, apply);
}

Json::Value OverlaySettingsServer::GetOverlaySettings(const boost::uuids::uuid& id, bool next) {
	std::lock_guard<std::recursive_mutex> l(m);
	Json::Value val = settings[id].GetSettings(next);
	val["id"] = boost::uuids::to_string(id);
	return val;
}

Json::Value OverlaySettingsServer::GetOverlaySettings() {
	std::lock_guard<std::recursive_mutex> l(m);
	Json::Reader reader;
	Json::Value root;
	Json::Value null;
	Json::Value title;

	for (auto i = settings.begin(); i != settings.end(); ++i) {
		root[boost::uuids::to_string(i->first)] = i->second.GetSettings(false);
	}
	Json::StyledWriter writer;
	return writer.write(root);
}
Json::Value OverlaySettingsServer::SetOverlaySettings(const Json::Value& root) {
	std::lock_guard<std::recursive_mutex> l(m);
	// TODO : CloseAll
	settings.clear();

	Json::FastWriter writer;

	for (auto i = root.begin(); i != root.end(); ++i) {
		auto id = boost::lexical_cast<boost::uuids::uuid>(i.key().asString());
		// TODO : Create
		settings[id].UpdateSettings(id, *i, true);
	}
	return root;
}

Json::Value OverlaySettingsServer::set(const Json::Value& value)
{
	Json::Value null;
	Json::Value id;
	try {
		if ((id = value.get("id", null)) != null) {
			boost::uuids::uuid id_ = boost::lexical_cast<boost::uuids::uuid>(id.asString());
			UpdateOverlaySettings(id_, value, true);
			return GetOverlaySettings(id_, true);
		}
		else {
			boost::uuids::uuid id_ = boost::uuids::random_generator()();
			UpdateOverlaySettings(id_, value, true);
			return GetOverlaySettings(id_, true);
		}
	}
	catch (std::runtime_error& e)
	{
		Json::Value error;
		error["error"] = e.what();
		return error;
	}
}
Json::Value OverlaySettingsServer::set_all(const Json::Value& value) {
	std::lock_guard<std::recursive_mutex> l(m);
	try {
		std::set<boost::uuids::uuid> current_ids, new_ids, intersection_ids;
		std::list<boost::uuids::uuid> temp_ids;
		{
			for (auto i = settings.begin(); i != settings.end(); ++i) {
				current_ids.insert(i->first);
			}
		}
		{
			for (auto i = value.begin(); i != value.end(); ++i) {
				new_ids.insert(boost::lexical_cast<boost::uuids::uuid>(i.key().asString()));
			}
		}

		std::set_intersection(current_ids.begin(), current_ids.end(), new_ids.begin(), new_ids.end(), std::back_inserter(temp_ids));

		{
			for (auto i = temp_ids.begin(); i != temp_ids.end(); ++i) {
				intersection_ids.insert(*i);
			}
		}
		for (auto i = current_ids.begin(); i != current_ids.end(); ++i)
		{
			if (intersection_ids.find(*i) == intersection_ids.end())
			{
				CloseOverlayWindow(*i);
			}
		}
		for (auto i = value.begin(); i != value.end(); ++i) {
			boost::uuids::uuid id_ = boost::lexical_cast<boost::uuids::uuid>(i.key().asString());
			UpdateOverlaySettings(id_, *i, true);
		}
		return value;
	}
	catch (std::runtime_error& e)
	{
		Json::Value error;
		error["error"] = e.what();
		return error;
	}
}

Json::Value OverlaySettingsServer::get(const Json::Value& value) {
	Json::Value null;
	Json::Value id;
	if ((id = value.get("id", null)) != null) {
		boost::uuids::uuid id_ = boost::lexical_cast<boost::uuids::uuid>(id.asString());
		return GetOverlaySettings(id_);
	}
	else {
		Json::Value error;
		error["error"] = "NotFound";
		return error;
	}
}

Json::Value OverlaySettingsServer::get_all() {
	std::lock_guard<std::recursive_mutex> l(m);
	Json::Value root;

	for (auto i = settings.begin(); i != settings.end(); ++i) {
		root[boost::uuids::to_string(i->first)] = GetOverlaySettings(i->first);
	}
	return root;
}

Json::Value OverlaySettingsServer::close(const Json::Value& value) {
	Json::Value null;
	Json::Value id;
	if ((id = value.get("id", null)) != null) {
		boost::uuids::uuid id_ = boost::lexical_cast<boost::uuids::uuid>(id.asString());
		CloseOverlayWindow(id_);
		return null;
	}
	else {
		Json::Value error;
		error["error"] = "NotFound";
		return error;
	}
}

Json::Value OverlaySettingsServer::close_all() {
	Json::Value null;
	{
		std::lock_guard<std::recursive_mutex> l(m);
		Json::Value root;

		while (settings.begin() != settings.end()) {
			CloseOverlayWindow(settings.begin()->first);
		}
		settings.clear();
	}
	return null;
}

Json::Value OverlaySettingsServer::capture(const Json::Value& value) {
	Json::Value null;
	Json::Value id;
	if ((id = value.get("id", null)) != null) {
		boost::uuids::uuid id_ = boost::lexical_cast<boost::uuids::uuid>(id.asString());
		return CaptureOverlayWindow(id_);
	}
	else {
		Json::Value error;
		error["error"] = "NotFound";
		return error;
	}
}