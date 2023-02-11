#include "pch.h"
#include "StatScraper.h"

using json = nlohmann::json;

BAKKESMOD_PLUGIN(StatScraper, "write a plugin description here", plugin_version, PLUGINTYPE_FREEPLAY)

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
	previousTotalPlayerPoints = 0;

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
	if (onlineGame.shouldUpdateRoster(gameWrapper)) {
		std::vector<Player> roster = getLiveRoster();
		onlineGame.roster = roster;
		sendOnlineGameToServer();
	}
}

bool StatScraper::shouldRun() {
	// Check to see if server exists
	ServerWrapper server = gameWrapper->GetOnlineGame();
	if (!server) { return false; }
	GameSettingPlaylistWrapper playlist = server.GetPlaylist();
	if (playlist.memory_address == NULL) { return false; }
	// we only care about duos right now (playlist 11)
	if (playlist.GetPlaylistId() != 11) { return false; }
	return true;
}


void StatScraper::sendOnlineGameToServer() {
	json jsonOnlineGame = onlineGame;
    CurlRequest req;             
    req.url = onlineGameUrl;
    req.body = jsonOnlineGame.dump();
    HttpWrapper::SendCurlJsonRequest(req, [this](int code, std::string result) {});
}

std::vector<Player> StatScraper::getLiveRoster() {
	std::vector<Player> players;
	ServerWrapper game = gameWrapper->GetOnlineGame();
	if (!game) { return players; }
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
		if (pri.GetPlayerID() == primaryPlayerID) {
			isPrimaryPlayer = true;
		}
		aPlayer.name = pri.GetPlayerName().ToString();
		aPlayer.bakkes_player_id = pri.GetPlayerID();
		aPlayer.platform_id_string = pri.GetUniqueIdWrapper().GetIdString();
		aPlayer.team_num = pri.GetTeamNum();
		aPlayer.score = pri.GetMatchScore();
		aPlayer.goals = pri.GetMatchGoals();
		aPlayer.saves = pri.GetMatchSaves();
		aPlayer.assists = pri.GetMatchAssists();
		aPlayer.shots = pri.GetMatchShots();
		aPlayer.mmr = mmrWrapper.GetPlayerMMR(pri.GetUniqueIdWrapper(), 11);
		aPlayer.is_primary_player = isPrimaryPlayer;
		players.push_back(aPlayer);
	}
	return players;
}


// TODO: Verify methods below and refactor to use JSON messages
void StatScraper::sendLog(std::string log_message) {
	//CurlRequest req;
	//json body;
	//body["event"] = "log_message";
	//body["message"] = log_message;
	//req.url = messageURL;
	//req.body = body.dump();
	//HttpWrapper::SendCurlJsonRequest(req, [this](int code, std::string result) {});
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
	primary_player_ending_mmr = primary_player_starting_mmr;
	PlayerControllerWrapper pc = gameWrapper->GetPlayerController();
	PriWrapper pri = pc.GetPRI();
	primary_bakkes_player_id = pri.GetPlayerID();
}

void OnlineGame::endGame(std::shared_ptr<GameWrapper> gameWrapper) {
	end_timestamp = getUnixTimestamp();
	for (Player p : roster) {
		if (p.is_primary_player) {
			primary_player_ending_mmr = p.mmr;
		}
	}
	game_state = GAME_ENDED;
}

int OnlineGame::getRosterTotalScore() {
	int totalScore = 0;
	for (Player p : roster) {
		totalScore += p.score;
	}
	return totalScore;
}

bool OnlineGame::shouldUpdateRoster(std::shared_ptr<GameWrapper> gameWrapper) {
	ServerWrapper game = gameWrapper->GetOnlineGame();
	if (!game) { return false; }
	ArrayWrapper<PriWrapper> priList = game.GetPRIs();
	int liveTotalScore = 0;
	int livePlayerCount = priList.Count();
	float livePrimaryPlayerMMR = getPlayerMMR(gameWrapper);
	for (int i = 0; i < priList.Count(); i++) {
		PriWrapper pri = priList.Get(i);
		liveTotalScore += pri.GetMatchScore();
	}
	if (liveTotalScore != getRosterTotalScore() or
		livePlayerCount != roster.size()) {
		return true;
	}
	if (!floatsAlmostEqual(livePrimaryPlayerMMR, primary_player_ending_mmr)) {
		primary_player_ending_mmr = livePrimaryPlayerMMR;
		return true;
	}
	return false;
}

void OnlineGame::resetGame() {
	roster.clear();
	match_id = ""; 
	start_timestamp = 0;
	end_timestamp = 0;
	primary_player_starting_mmr = 0.0;
	primary_player_ending_mmr = 0.0;
	primary_bakkes_player_id = 0;
	game_state = NO_GAME;
}

float OnlineGame::getPlayerMMR(std::shared_ptr<GameWrapper> gameWrapper) {
	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	PlayerControllerWrapper pc = gameWrapper->GetPlayerController();
	PriWrapper pri = pc.GetPRI();
	if (!pc) { return 0.0; }
	if (!pri) { return 0.0; }
	return mmrWrapper.GetPlayerMMR(pri.GetUniqueIdWrapper(), DUALS_PLAYLIST_ID);
}


void OnlineGame::updateRoster(std::vector<Player> roster) {

}