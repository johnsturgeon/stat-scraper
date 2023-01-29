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


void StatScraper::onLoad()
{
	_globalCvarManager = cvarManager;
	previousTotalPlayerPoints = 0;
	LOG("JHS: Plugin loaded!");

	cvarManager->registerCvar("roster_sent", "0");
	cvarManager->registerCvar("in_replay", "0");

	gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.Active.StartRound",
		[this](std::string eventname) {
			startRound();
		});

	gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
		[this](std::string eventname) {
			matchEnded();
		});
	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed",
		[this](std::string eventname) {
			gameDestroyed();
		});

	//  We need the params so we hook with caller, but there is no wrapper for the HUD
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
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.ReplayPlayback.BeginState",
		[this](std::string eventname) {
			replayStarted();
		});
	gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.ReplayPlayback.EndState",
		[this](std::string eventname) {
			replayEnded();
		});
	gameWrapper->HookEvent("Function GameEvent_Soccar_TA.PodiumSpotlight.BeginState",
		[this](std::string eventname) {
			sendLog(eventname);
			sendAllPlayerPriStatsToServer("final_stats", eventname);
		});
	gameWrapper->RegisterDrawable(
		[this](CanvasWrapper canvas) {
			handleTick();
		}
	);

}


void StatScraper::updateMMRStats(UniqueIDWrapper id) {
	if (!gameWrapper) { return; }
	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	float mmr = mmrWrapper.GetPlayerMMR(id, 11);
	PlayerControllerWrapper pc = gameWrapper->GetPlayerController();
	if (!pc) { return; }
	PriWrapper pri = pc.GetPRI();
	if (!pri) { return; }
	UniqueIDWrapper primaryID = pri.GetUniqueIdWrapper();

	if (id == primaryID) {
		sendLog("GoshDarnedHero's MMR is : " + std::to_string(mmr));
	}
	
}

int StatScraper::getPlayerCount(ArrayWrapper<PriWrapper> priList) {
	static int previousPlayerCount = 0;
	int playerCount = priList.Count();
	if (playerCount > 0 && playerCount < 4) {
		if (playerCount != previousPlayerCount) {
			LOG("Player Count: {}", playerCount);
			previousPlayerCount = playerCount;
		}
	}
	return playerCount;
}

bool StatScraper::shouldRun() {
	// Check to see if server exists
	ServerWrapper server = gameWrapper->GetOnlineGame();
	if (!server) {
		if (gameStarted) {
			LOG("No online game!");
			gameStarted = false;
		}
		return false;
	}
	GameSettingPlaylistWrapper playlist = server.GetPlaylist();
	if (playlist.memory_address == NULL) {
		return false;
	}
	if (playlist.GetPlaylistId() != 11) {
		return false;
	}
	return true;
}


int StatScraper::getTotalPoints() {
	ServerWrapper game = gameWrapper->GetOnlineGame();
	if (!game) { return 0; }
	ArrayWrapper<PriWrapper> priList = game.GetPRIs();
	int player_count = getPlayerCount(priList);
	if (player_count < 4) { return 0; }
	int totalScore = 0;
	for (int i = 0; i < player_count; i++) {
		PriWrapper pri = priList.Get(i);
		totalScore += pri.GetMatchScore();
	}
	return totalScore;
}

void StatScraper::handleTick() {
	if (shouldRun() == false) {
		return;
	}
	int liveTotalPoints = getTotalPoints();
	if (liveTotalPoints != previousTotalPlayerPoints) {
		previousTotalPlayerPoints = liveTotalPoints;
		sendAllPlayerPriStatsToServer("updated_stats", "game_time_tick");
	}
}

void StatScraper::sendAllPlayerPriStatsToServer(std::string event, std::string sender) {
	// return if the roster is not set yet
	ServerWrapper game = gameWrapper->GetOnlineGame();
	if (!game) { return; }
	if (event == "initial_roster") {
		PlayerControllerWrapper pc = gameWrapper->GetPlayerController();
		if (!pc) { return; }
		PriWrapper pri = pc.GetPRI();
		if (!pri) { return; }
		primaryPlayerID = pri.GetUniqueIdWrapper();
		gameStarted = true;
	}
	ArrayWrapper<PriWrapper> priList = game.GetPRIs();
	int player_count = getPlayerCount(priList);
	if (player_count < 4) { return; }
	MMRWrapper mmrWrapper = gameWrapper->GetMMRWrapper();
	json body;
	body["event"] = event;
	body["sender"] = sender;
	for (int i = 0; i < player_count; i++) {
		PriWrapper pri = priList.Get(i);
		int isPrimaryPlayer = 0;
		if (pri.GetUniqueIdWrapper() == primaryPlayerID) {
			isPrimaryPlayer = 1;
		}
		body["players"][i]["name"] = pri.GetPlayerName().ToString();
		body["players"][i]["bakkes_player_id"] = pri.GetPlayerID();
		body["players"][i]["platform_id_string"] = pri.GetUniqueIdWrapper().GetIdString();
		body["players"][i]["team"] = pri.GetTeamNum();
		body["players"][i]["score"] = pri.GetMatchScore();
		body["players"][i]["goals"] = pri.GetMatchGoals();
		body["players"][i]["saves"] = pri.GetMatchSaves();
		body["players"][i]["assists"] = pri.GetMatchAssists();
		body["players"][i]["shots"] = pri.GetMatchShots();
		body["players"][i]["mmr"] = mmrWrapper.GetPlayerMMR(pri.GetUniqueIdWrapper(), 11);
		body["players"][i]["is_primary_player"] = isPrimaryPlayer;
	}
	sendServerEvent(body);
}

void StatScraper::matchEnded() {
	auto roster_sent = cvarManager->getCvar("roster_sent");
	roster_sent.setValue("0");
}

void StatScraper::gameDestroyed() {
	sendAllPlayerPriStatsToServer("final_stats", "game_destroyed");
	auto roster_sent = cvarManager->getCvar("roster_sent");
	roster_sent.setValue("0");
}

void StatScraper::startRound() {
	LOG("Got Start Round");
	ServerWrapper game = gameWrapper->GetOnlineGame();
	if (!game) { return; }

	ArrayWrapper<PriWrapper> priList = game.GetPRIs();
	int player_count = getPlayerCount(priList);
	LOG("Player Count: {}",player_count);
	if (player_count < 4) { return; }

	auto roster_sent = cvarManager->getCvar("roster_sent");
	if (roster_sent.getBoolValue()) { return; }

	roster_sent.setValue("1");

	sendAllPlayerPriStatsToServer("initial_roster", "start_round");
	currentMatchGUID = game.GetMatchGUID();  

}


void StatScraper::sendStatEvent(std::string event_type, PriWrapper pri, StatEventWrapper stat_event) {
	if (primaryPlayerID != pri.GetUniqueIdWrapper()) { return; }
	json body;
	body["event"] = "stat_event";
	body["event_name"] = stat_event.GetEventName();
	body["event_points"] = stat_event.GetPoints();
	body["event_description"] = stat_event.GetDescription().ToString();
	body["event_label"] = stat_event.GetLabel().ToString();
	body["event_primary_stat"] = stat_event.GetbPrimaryStat();
	sendServerEvent(body);
}

void StatScraper::sendPriStats(std::string event_type, PriWrapper pri, std::string event_name) {
	//if (pri.GetPlayerID() != primaryPlayerID) { return; }
	json body;
	body["event"] = event_type;
	body["event_name"] = event_name;
	sendServerEvent(body);
}

void StatScraper::replayStarted() {
	auto in_replay = cvarManager->getCvar("in_replay");
	in_replay.setValue("1");
	json body;
	body["event"] = "replay_started";
	sendServerEvent(body);
	gameWrapper->SetTimeout([this](GameWrapper* gw)
		{
			sendAllPlayerPriStatsToServer("updated_stats", "delayed_during_replay");
		}, 0.75f);
}

void StatScraper::replayEnded() {
	auto in_replay = cvarManager->getCvar("in_replay");
	in_replay.setValue("0");
	json body;
	body["event"] = "replay_ended";
	sendServerEvent(body);
}

void StatScraper::sendServerEvent(json body) {
	CurlRequest req;
	req.url = "http://imac:8822/bakkes";
	req.body = body.dump();

	HttpWrapper::SendCurlJsonRequest(req, [this](int code, std::string result)
		{
			
		});

}

void StatScraper::sendLog(std::string log_message) {
	json body;
	body["event"] = "log_message";
	body["message"] = log_message;
	sendServerEvent(body);
}

void StatScraper::handleStatEvent(void* params) {
	StatEventParams* pStruct = (StatEventParams*)params;
	StatEventWrapper statEvent = StatEventWrapper(pStruct->StatEvent);
	PlayerControllerWrapper playerController = gameWrapper->GetPlayerController();
	if (!playerController) { LOG("Null controller"); return; }
	PriWrapper playerPRI = playerController.GetPRI();
	sendLog("handleStatEvent is sending");
	sendStatEvent("stat_event", playerPRI, statEvent);
}

void StatScraper::onStatTickerMessage(void* params) {
	struct StatTickerParams {
		uintptr_t Receiver;
		uintptr_t Victim;
		uintptr_t StatEvent;
	};

	StatTickerParams* pStruct = (StatTickerParams*)params;
	PriWrapper receiver = PriWrapper(pStruct->Receiver);
	PriWrapper victim = PriWrapper(pStruct->Victim);
	StatEventWrapper statEvent = StatEventWrapper(pStruct->StatEvent);
	if (!receiver) { LOG("Null reciever PRI"); return; }
	this->sendStatEvent("stat_ticker", receiver, statEvent);
}

// ---------------------------  OnlineGame methods --------------------


void OnlineGame::gameKickoff() {
	if (!game) { return; }
	// get the player count (just testing rn)
	ArrayWrapper<PriWrapper> priList = game.GetPRIs();
	int playerCount = priList.Count();
	CurlRequest req;
	req.url = "http://imac:8822/bakkes";
	json body;
	body["event"] = "OnlineGame::gameKickoff";
	body["player_count"] = playerCount;

	req.body = body.dump();

	HttpWrapper::SendCurlJsonRequest(req, [this](int code, std::string result) {

	});

}