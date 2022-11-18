#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "json.hpp"
#include "version.h"

using json = nlohmann::json;

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);


class StatScraper: public BakkesMod::Plugin::BakkesModPlugin
	//,public SettingsWindowBase // Uncomment if you wanna render your own tab in the settings menu
	//,public PluginWindowBase // Uncomment if you want to render your own plugin window
{

	//std::shared_ptr<bool> enabled;

	//Boilerplate
	void onLoad() override;
	//void onUnload() override; // Uncomment and implement if you need a unload method

	void onStatTickerMessage(void*);
	void onHitBall(CarWrapper, void*);
	void startRound();
	void gameTimeTick();
	void matchEnded();
	void replayStarted();
	void replayEnded();
	void sendServerEvent(json);
	void sendPriStats(PriWrapper, std::string);
	void registerPlayer(ActorWrapper caller);
	void registerPlayerWithCaller(PriXWrapper,  void*);


public:
	//void RenderSettings() override; // Uncomment if you wanna render your own tab in the settings menu
	//void RenderWindow() override; // Uncomment if you want to render your own plugin window
};
