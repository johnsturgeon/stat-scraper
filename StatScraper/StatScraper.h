#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "json.hpp"
#include "version.h"

#define FULL_ROSTER = 11;

using json = nlohmann::json;

enum GameState {GameEnded, GameStarted};

class Player {
public:
	std::string name;
	int bakkes_player_id = 0;
	std::string platform_id_string;
	int team_num = 0;
	int score = 0;
	int goals = 0;
	int saves = 0;
	int assists = 0;
	int shots = 0;
	float mmr = 0.0;
	bool is_primary_player = false;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        Player, name, bakkes_player_id, platform_id_string, team_num,
                score, goals, saves, assists, shots, mmr, is_primary_player)

class OnlineGame {

public:
	void startGame();
	void endGame();
	json getRosterJSON();
	std::vector<Player> roster;
	GameState gameState;
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
{	
	/*
	Refactored and used methods
	*/
	void onLoad() override;
	void sendServerJSON(json);
	void sendLog(std::string);
	bool shouldRun();
	void handleTick();
	std::vector<Player> getLiveRoster();
	void sendRosterToServer(std::string, std::string);

	/*
	TODO Figure out what's used and not
	*/

	void onStatTickerMessage(void*);
	void startRound();
	void matchEnded();
	void gameDestroyed();
	void replayStarted();
	void replayEnded();
	int getTotalPoints();
	void handleStatEvent(void*);
	void updateMMRStats(UniqueIDWrapper);
	void sendServerEvent(json);
	void sendStatEvent(std::string, PriWrapper, StatEventWrapper);
	int getPlayerCount(ArrayWrapper<PriWrapper> priList);
	void sendAllPlayerPriStatsToServer(std::string, std::string);
	void sendPriStats(std::string event_type, PriWrapper, std::string event_name);


public:
	//void RenderSettings() override; // Uncomment if you wanna render your own tab in the settings menu
	//void RenderWindow() override; // Uncomment if you want to render your own plugin window

private:

	/*
	Refactored and Used variables
	*/
	OnlineGame onlineGame;
	int previousTotalPlayerPoints;
	int previousPlayerCount;

	/*
	TODO figure out
	*/
	std::string currentMatchGUID;
	UniqueIDWrapper primaryPlayerID;
    std::string baseURL = "http://imac:8822/";
//  std::string baseURL = "http://goshdarnedserver:8822/"
    std::string rosterURL = baseURL + "roster"
};
