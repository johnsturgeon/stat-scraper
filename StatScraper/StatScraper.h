#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "json.hpp"
#include "version.h"

using json = nlohmann::json;


class OnlineGame {
public:
	ServerWrapper game;
	void gameKickoff();
};


struct StatEventParams {
	// always primary player
	uintptr_t PRI;
	// wrapper for the stat event
	uintptr_t StatEvent;
};

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);
std::unique_ptr<MMRNotifierToken> notifierToken;

class StatScraper: public BakkesMod::Plugin::BakkesModPlugin
	//,public SettingsWindowBase // Uncomment if you wanna render your own tab in the settings menu
	//,public PluginWindowBase // Uncomment if you want to render your own plugin window
{	

	//std::shared_ptr<bool> enabled;

	//Boilerplate
	void onLoad() override;
	//void onUnload() override; // Uncomment and implement if you need a unload method

	void sendLog(std::string);
	void onStatTickerMessage(void*);
	void startRound();
	void gameTimeTick();
	void matchEnded();
	void gameDestroyed();
	void replayStarted();
	void replayEnded();
	bool shouldRun();
	void handleTick();
	int getTotalPoints();
	void handleStatEvent(void*);
	void updateMMRStats(UniqueIDWrapper);
	void sendServerEvent(json);
	int getPlayerCount(ArrayWrapper<PriWrapper> priList);
	void sendAllPlayerPriStatsToServer(std::string, std::string);
	void sendPriStats(std::string event_type, PriWrapper, std::string event_name);


public:
	//void RenderSettings() override; // Uncomment if you wanna render your own tab in the settings menu
	//void RenderWindow() override; // Uncomment if you want to render your own plugin window

private:
	std::string currentMatchGUID;
	UniqueIDWrapper primaryPlayerID;
	int previousTotalPlayerPoints;
	int gameStarted;
};
