#pragma once

#include <chrono>
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "json.hpp"
#include "version.h"

const int DUALS_PLAYLIST_ID = 11;

using json = nlohmann::json;

enum GameState {
	NO_GAME = 0,
	GAME_IN_PROCESS = 1,
	GAME_ENDED = 2
};

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
private:
	float getPlayerMMR(std::shared_ptr<GameWrapper>);
	int getRosterTotalScore();

public:
	void startGame(std::shared_ptr<GameWrapper>);
	void endGame(std::shared_ptr<GameWrapper>);
	bool shouldUpdateRoster(std::shared_ptr<GameWrapper> gameWrapper);
	void updateRoster(std::vector<Player> roster);
	void resetGame();
	std::string match_id;
	int start_timestamp = 0;
	int end_timestamp = 0;
	std::vector<Player> roster;
	float primary_player_starting_mmr = 0.0;
	float primary_player_ending_mmr = 0.0;
	int primary_bakkes_player_id = 0;
	GameState game_state = NO_GAME;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
	OnlineGame, match_id, start_timestamp, end_timestamp, roster, primary_player_starting_mmr,
	primary_player_ending_mmr, primary_bakkes_player_id, game_state)


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
	void sendLog(std::string);
	bool shouldRun();
	void handleTick();
	std::vector<Player> getLiveRoster();
	void sendOnlineGameToServer();

	/*
	TODO Figure out what's used and not
	*/

	void onStatTickerMessage(void*);
	void handleStatEvent(void*);
	void sendStatEvent(std::string, PriWrapper, StatEventWrapper);


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
    std::string baseUrl = "http://imac:8822/";
//  std::string baseUrl = "http://goshdarnedserver:8822/"
	std::string onlineGameUrl = baseUrl + "online_game";
};
