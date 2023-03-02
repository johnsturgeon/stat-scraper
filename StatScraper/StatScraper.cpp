#include "pch.h"
#include "StatScraper.h"

using json = nlohmann::json;

BAKKESMOD_PLUGIN(StatScraper, "Stat Tracker", plugin_version, PLUGINTYPE_FREEPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

struct DummyStatEventContainer
{
	uintptr_t Receiver;
	uintptr_t Victim;
	uintptr_t StatEvent;
};

struct StatTickerParams {
	// person who got a stat
	uintptr_t Receiver;
	// person who is victim of a stat (only exists for demos afaik)
	uintptr_t Victim;
	// wrapper for the stat event
	uintptr_t StatEvent;
};

// ----------- Global Utility Functions --------------

int getUnixTimestamp() {
    const auto t1 = std::chrono::system_clock::now();
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(t1.time_since_epoch()).count();
	return static_cast<int>(seconds);
}

bool floatsAlmostEqual(float a, float b) {
	if (abs(a - b) < 0.01f) {
		return true;
	}
	return false;
}

// Overridden from Bakkes
void StatScraper::onLoad()
{
	_globalCvarManager = cvarManager;
	_globalCvarManager->registerCvar("base_url", defaultBaseUrl, "Override the default server URL");

	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage",
		[this](ServerWrapper caller, void* params, std::string eventname) {
			onStatTickerMessage(params);
		});
	// hooked whenever the primary player earns a stat
	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatEvent",
		[this](ServerWrapper caller, void* params, std::string eventname) {
			handleStatEvent(params);
		});
	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage",
		[this](ServerWrapper caller, void* params, std::string eventname) {
			onStatTickerMessage(params);
		});

	/*
	Refactored and used hooks
	*/
	gameWrapper->RegisterDrawable(
		[this](CanvasWrapper canvas) {
			handleTick();
		}
	);

}

// Overridden from Bakkes
void StatScraper::RenderSettings() {
	ImGui::TextUnformatted("A really cool plugin");
	CVarWrapper baseURLCvar = cvarManager->getCvar("base_url");
	if (!baseURLCvar) { return; }
	std::string baseURL = baseURLCvar.getStringValue();
	if (ImGui::InputText("Override the default URL", &baseURL)) {
		baseURLCvar.setValue(baseURL);
	}
}
/*
Refactored and used methods below
*/

void StatScraper::handleTick() {
	// First we determine if we're in an online game, or not, if not, just bail
	//
	if (shouldRun() == false) {
		if (onlineGame.game_state == GAME_IN_PROCESS) {
			onlineGame.endGame(gameWrapper);
			sendOnlineGameToServer();
			previousTotalPlayerPoints = 0;
			onlineGame.resetGame();
		}
		return;
	}
	// if this is the first tick after a game has started, let's init the 
	// onlineGame and post the new game to the server
	if (onlineGame.game_state != GAME_IN_PROCESS) {
		previousPlayerCount = 0;
		previousTotalPlayerPoints = 0;
		onlineGame.startGame(gameWrapper);
		sendOnlineGameToServer();
	}
	bool didUpdateGame = onlineGame.updateGameIfNecessary(gameWrapper);
	if (didUpdateGame) {
		sendOnlineGameToServer();
	}
}

bool StatScraper::shouldRun() {
	// Check to see if server exists
	ServerWrapper server = gameWrapper->GetOnlineGame();
	if (!server) { return false; } 
	int currentPlaylist = getCurrentPlaylistInt(server);
	if (!playlistIsValid(currentPlaylist)) { return false; }
	return true;
}

int StatScraper::getCurrentPlaylistInt(ServerWrapper server) {
	GameSettingPlaylistWrapper playlist = server.GetPlaylist();
	if (playlist.memory_address == NULL) { return -1; }
	return playlist.GetPlaylistId();
}

void StatScraper::sendOnlineGameToServer() {
	json jsonOnlineGame = onlineGame;
    CurlRequest req;             
    req.url = onlineGameUrl();
	req.verb = "POST";
	req.body = jsonOnlineGame.dump();
    HttpWrapper::SendCurlJsonRequest(req, [this](int code, std::string result) {
		LOG("Result: {}", result);
		});
}
bool StatScraper::playlistIsValid(int idToCheck) {
	std::vector<PlaylistIds> validPlaylists;
	validPlaylists.push_back(PlaylistIds::RankedTeamDoubles);

	for (PlaylistIds id : validPlaylists) {
		int idInt = static_cast<int>(id);
		if (idInt == idToCheck) {
			return true;
		}
	} 
	return false;
}

std::string StatScraper::onlineGameUrl() {
	CVarWrapper baseURLCvarOverride = cvarManager->getCvar("base_url");
	if (baseURLCvarOverride) {
		return baseURLCvarOverride.getStringValue() + "online_game";
	}
	return defaultBaseUrl + "online_game";
}

void StatScraper::sendStatEvent(std::string event_type, PriWrapper pri, StatEventWrapper stat_event) {
	//if (primaryPlayerID != pri.GetUniqueIdWrapper()) { return; }
	//json body;
	//body["event"] = "stat_event";
	//body["event_name"] = stat_event.GetEventName();
	//body["event_points"] = stat_event.GetPoints();
	//body["event_description"] = stat_event.GetDescription().ToString();
	//body["event_label"] = stat_event.GetLabel().ToString();
	//body["event_primary_stat"] = stat_event.GetbPrimaryStat();
	//sendServerEvent(body);
}

void StatScraper::handleStatEvent(void* params) {
	//StatEventParams* pStruct = (StatEventParams*)params;
	//StatEventWrapper statEvent = StatEventWrapper(pStruct->StatEvent);
	//PlayerControllerWrapper playerController = gameWrapper->GetPlayerController();
	//if (!playerController) { return; }
	//PriWrapper playerPRI = playerController.GetPRI();
	//sendLog("handleStatEvent is sending");
	//sendStatEvent("stat_event", playerPRI, statEvent);
}

void StatScraper::onStatTickerMessage(void* params) {
	//struct StatTickerParams {
	//	uintptr_t Receiver;
	//	uintptr_t Victim;
	//	uintptr_t StatEvent;
	//};

	//StatTickerParams* pStruct = (StatTickerParams*)params;
	//PriWrapper receiver = PriWrapper(pStruct->Receiver);
	//PriWrapper victim = PriWrapper(pStruct->Victim);
	//StatEventWrapper statEvent = StatEventWrapper(pStruct->StatEvent);
	//if (!receiver) { return; }
	//this->sendPriStats("stat_ticker", receiver, statEvent.GetEventName());
}


// ---------------------------  OnlineGame methods --------------------
void OnlineGame::startGame(std::shared_ptr<GameWrapper> gameWrapper) {
	ServerWrapper game = gameWrapper->GetOnlineGame();
	game_state = GAME_IN_PROCESS;
	match_id = game.GetMatchGUID();
	start_timestamp = getUnixTimestamp();
	primary_player_starting_mmr = getPlayerMMR(gameWrapper);
	PlayerControllerWrapper pc = gameWrapper->GetPlayerController();
	PriWrapper pri = pc.GetPRI();
	primary_bakkes_player_id = pri.GetPlayerID();
	playlist_id = StatScraper::getCurrentPlaylistInt(game);
}

void OnlineGame::endGame(std::shared_ptr<GameWrapper> gameWrapper) {
	game_state = GAME_ENDED;
}

int OnlineGame::getRosterTotalScore() {
	int totalScore = 0;
	for (auto const& [key, val] : rosterMap) {
		if (val.is_in_game) {
			totalScore += val.score;
		}
	}
	return totalScore;
}

int OnlineGame::getRosterCount() {
	int count = 0;
	for (auto const& [key, val] : rosterMap) {
		if (val.is_in_game) {
			count += 1;
		}
	}
	return count;
}

bool OnlineGame::updateGameIfNecessary(std::shared_ptr<GameWrapper> gameWrapper) {
	ServerWrapper game = gameWrapper->GetOnlineGame();
	if (!game) { return false; }

	// first let's check to see if it's a 'roster' update
	ArrayWrapper<PriWrapper> priList = game.GetPRIs();
	int livePlayerCount = priList.Count();
	if (livePlayerCount != getRosterCount()) {
		updateRoster(gameWrapper);
		return true;
	}
	
	// now let's see if the score changed
	int liveTotalScore = 0;
	for (int i = 0; i < priList.Count(); i++) {
		PriWrapper pri = priList.Get(i);
		liveTotalScore += pri.GetMatchScore();
	}
	if (liveTotalScore != getRosterTotalScore()) {
		updateRoster(gameWrapper);
		return true;
	}

	// now check to see if mmr has changed
	float livePrimaryPlayerMMR = getPlayerMMR(gameWrapper);
	if (!floatsAlmostEqual(livePrimaryPlayerMMR, primary_player_ending_mmr())) {
		updateRoster(gameWrapper);
		return true;
	}
	return false;
}

void OnlineGame::resetGame() {
	rosterMap.clear();
	match_id = ""; 
	start_timestamp = 0;
	last_update_timestamp = 0;
	primary_player_starting_mmr = 0.0;
	primary_bakkes_player_id = 0;
	game_state = NO_GAME;
}

float OnlineGame::getPlayerMMR(std::shared_ptr<GameWrapper> gameWrapper) {
	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();  
	PlayerControllerWrapper pc = gameWrapper->GetPlayerController();
	PriWrapper pri = pc.GetPRI();
	if (!pc) { return 0.0; }
	if (!pri) { return 0.0; }
	return mmrWrapper.GetPlayerMMR(pri.GetUniqueIdWrapper(), static_cast<int>(PlaylistIds::RankedTeamDoubles));
}

void OnlineGame::updateRoster(std::shared_ptr<GameWrapper> gameWrapper) {
	// first let's set the player's 'in_game' to false for all the players
	for (auto const& [key, val] : rosterMap) {
		rosterMap[key].is_in_game = false;
	}
	this->last_update_timestamp = getUnixTimestamp();
	ServerWrapper game = gameWrapper->GetOnlineGame();
	if (!game) { return; }
	ArrayWrapper<PriWrapper> priList = game.GetPRIs();
	int player_count = priList.Count();
	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	PlayerControllerWrapper pc = gameWrapper->GetPlayerController();
	PriWrapper primaryPRI = pc.GetPRI();
	int primaryPlayerID = primaryPRI.GetPlayerID();
	for (int i = 0; i < player_count; i++) {
		PriWrapper pri = priList.Get(i);
		Player aPlayer = Player();
		bool isPrimaryPlayer = false;
		std::string platform_id_string = pri.GetUniqueIdWrapper().GetIdString();
		if (pri.GetPlayerID() == primaryPlayerID) {
			isPrimaryPlayer = true;
		}
		aPlayer.name = pri.GetPlayerName().ToString();
		aPlayer.bakkes_player_id = pri.GetPlayerID();
		aPlayer.platform_id_string = platform_id_string;
		aPlayer.team_num = pri.GetTeamNum();
		aPlayer.score = pri.GetMatchScore();
		aPlayer.goals = pri.GetMatchGoals();
		aPlayer.saves = pri.GetMatchSaves();
		aPlayer.assists = pri.GetMatchAssists();
		aPlayer.shots = pri.GetMatchShots();
		aPlayer.mmr = mmrWrapper.GetPlayerMMR(pri.GetUniqueIdWrapper(), static_cast<int>(PlaylistIds::RankedTeamDoubles));
		aPlayer.skill_rank = mmrWrapper.GetPlayerRank(pri.GetUniqueIdWrapper(), static_cast<int>(PlaylistIds::RankedTeamDoubles));
		aPlayer.is_primary_player = isPrimaryPlayer;
		aPlayer.is_in_game = true;
		rosterMap[platform_id_string] = aPlayer;
	}
}

// properties
std::vector<Player> OnlineGame::roster() const {
	std::vector<Player> rosterVector;
	for (auto const& [key, val] : rosterMap) {
		rosterVector.push_back(val);
	}
	return rosterVector;
}

int OnlineGame::end_timestamp() const {
	return getUnixTimestamp();
}

float OnlineGame::primary_player_ending_mmr() const {
	return primary_player().mmr;
}

Player OnlineGame::primary_player() const {
	for (auto const& [key, val] : rosterMap) {
		if (val.is_primary_player) {
			return val;
		}
	}
	return Player();
}
