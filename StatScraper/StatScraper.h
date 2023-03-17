#pragma once

#include <chrono>
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "json.hpp"
#include "version.h"
#include "GuiBase.h"

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
	SkillRank skill_rank = { 0,0,0 };
	bool is_primary_player = false;
	bool is_in_game = false;
};

void to_json(json& j, const Player& p) {
	j = json {
		{"name", p.name},
		{"bakkes_player_id", p.bakkes_player_id},
		{"platform_id_string", p.platform_id_string},
		{"team_num", p.team_num},
		{"is_in_game", p.is_in_game},
		{"score", p.score},
		{"goals", p.goals},
		{"saves", p.saves},
		{"assists", p.assists},
		{"shots", p.shots},
		{"mmr", p.mmr},
		{"is_primary_player", p.is_primary_player},
		{"skill_rank", {
			{"tier", p.skill_rank.Tier},
			{"division", p.skill_rank.Division},
			{"matches_played", p.skill_rank.MatchesPlayed}
			}
		}
	};
}


class OnlineGame {
private:
	float getPlayerMMR(std::shared_ptr<GameWrapper>);
	int getRosterTotalScore();
	int getRosterCount();
	void updateRoster(std::shared_ptr<GameWrapper> gameWrapper);
	std::map<std::string, Player> rosterMap;
	Player primary_player() const;

public:
	void startGame(std::shared_ptr<GameWrapper>);
	void endGame(std::shared_ptr<GameWrapper>);
	bool updateGameIfNecessary(std::shared_ptr<GameWrapper>);
	void resetGame();
	std::string match_id;
	int start_timestamp = 0;
	int playlist_id = 0;
	int last_update_timestamp = 0;
	float primary_player_starting_mmr = 0.0;
	int primary_bakkes_player_id = 0;
	GameState game_state = NO_GAME;
	int end_timestamp() const;
	float primary_player_ending_mmr() const;
	std::vector<Player> roster() const;
};

void to_json(json& j, const OnlineGame& g) {
	j = json{
		{"match_id", g.match_id},
		{"start_timestamp", g.start_timestamp},
		{"end_timestamp", g.end_timestamp()},
		{"last_update_timestamp", g.last_update_timestamp},
		{"roster", g.roster()},
		{"primary_player_starting_mmr", g.primary_player_starting_mmr},
		{"primary_player_ending_mmr", g.primary_player_ending_mmr()},
		{"primary_bakkes_player_id", g.primary_bakkes_player_id},
		{"game_state", g.game_state},
		{"playlist_id", g.playlist_id}
	};
}

struct ChatMessage {
	std::string match_id;
	int timestamp;
	int channel;
	std::string message;
	std::string player_name;
};

void to_json(json& j, const ChatMessage& m) {
	j = json{
		{"match_id", m.match_id},
		{"timestamp", m.timestamp},
		{"channel", m.channel},
		{"message", m.message},
		{"player_name", m.player_name}
	};
}

struct StatEventParams {
	// always primary player
	uintptr_t PRI;
	// wrapper for the stat event
	uintptr_t StatEvent;
};

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);
std::unique_ptr<MMRNotifierToken> notifierToken;

class StatScraper: public BakkesMod::Plugin::BakkesModPlugin, public SettingsWindowBase
{	
	/*
	Refactored and used methods
	*/
	void onLoad() override;
	void RenderSettings() override;

	void sendLog(std::string);
	bool shouldRun();
	void handleTick();
	void sendOnlineGameToServer();
	bool playlistIsValid(int);
	/*
	TODO Figure out what's used and not
	*/

	void onStatTickerMessage(void*);
	void handleStatEvent(void*);
	void handleChatMessage(void*);
	void sendStatEvent(std::string, PriWrapper, StatEventWrapper);


public:
	//void RenderSettings() override; // Uncomment if you wanna render your own tab in the settings menu
	//void RenderWindow() override; // Uncomment if you want to render your own plugin window
	static int  getCurrentPlaylistInt(ServerWrapper);

private:

	/*
	Refactored and Used variables
	*/
	OnlineGame onlineGame;
	int previousTotalPlayerPoints;
	int previousPlayerCount;
	std::string onlineGameUrl();
	std::string chatMessageUrl();

	/*
	TODO figure out
	*/
	std::string currentMatchGUID;
	UniqueIDWrapper primaryPlayerID;
    std::string defaultBaseUrl = "http://localhost:8822/";
};
