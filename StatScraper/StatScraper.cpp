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

	LOG("JHS: Plugin loaded!");
	// !! Enable debug logging by setting DEBUG_LOG = true in logging.h !!
	//DEBUGLOG("StatScraper debug mode enabled");

	// LOG and DEBUGLOG use fmt format strings https://fmt.dev/latest/index.html
	//DEBUGLOG("1 = {}, 2 = {}, pi = {}, false != {}", "one", 2, 3.14, true);

	//cvarManager->registerNotifier("my_aweseome_notifier", [&](std::vector<std::string> args) {
	//	LOG("Hello notifier!");
	//}, "", 0);

	//auto cvar = cvarManager->registerCvar("template_cvar", "hello-cvar", "just a example of a cvar");
	//auto cvar2 = cvarManager->registerCvar("template_cvar2", "0", "just a example of a cvar with more settings", true, true, -10, true, 10 );

	//cvar.addOnValueChanged([this](std::string cvarName, CVarWrapper newCvar) {
	//	LOG("the cvar with name: {} changed", cvarName);
	//	LOG("the new value is: {}", newCvar.getStringValue());
	//});

	//cvar2.addOnValueChanged(std::bind(&StatScraper::YourPluginMethod, this, _1, _2));

	// enabled decleared in the header
	//enabled = std::make_shared<bool>(false);
	cvarManager->registerCvar("roster_sent", "0");
	cvarManager->registerCvar("in_replay", "0");
	//cvarManager->registerCvar("TEMPLATE_Enabled", "0", "Enable the TEMPLATE plugin", true, true, 0, true, 1).bindTo(enabled);

	//cvarManager->registerNotifier("NOTIFIER", [this](std::vector<std::string> params){FUNCTION();}, "DESCRIPTION", PERMISSION_ALL);
	//cvarManager->registerCvar("CVAR", "DEFAULTVALUE", "DESCRIPTION", true, true, MINVAL, true, MAXVAL);//.bindTo(CVARVARIABLE);
	//gameWrapper->HookEvent("FUNCTIONNAME", std::bind(&TEMPLATE::FUNCTION, this));
	//gameWrapper->HookEventWithCallerPost<ActorWrapper>("FUNCTIONNAME", std::bind(&StatScraper::FUNCTION, this, _1, _2, _3));
	//gameWrapper->RegisterDrawable(bind(&TEMPLATE::Render, this, std::placeholders::_1));

	gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnGameTimeUpdated",
		[this](std::string eventname) {
			gameTimeTick();
		});
	gameWrapper->HookEventPost("Function GameEvent_Soccar_TA.Active.StartRound",
		[this](std::string eventname) {
			startRound();
		});

	gameWrapper->HookEventPost("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
		[this](std::string eventname) {
			matchEnded();
		});
	//  We need the params so we hook with caller, but there is no wrapper for the HUD
	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatTickerMessage",
		[this](ServerWrapper caller, void* params, std::string eventname) {
			onStatTickerMessage(params);
		});

	// hooked whenever the primary player earns a stat
	gameWrapper->HookEventWithCallerPost<ServerWrapper>("Function TAGame.GFxHUD_TA.HandleStatEvent",
		[this](ServerWrapper caller, void* params, std::string eventname) {
			//onStatEvent(params);
		});
	//gameWrapper->HookEvent("Function TAGame.Ball_TA.Explode", [this](std::string eventName) {
	//	LOG("Your hook got called and the ball went POOF");
	//});
	// You could also use std::bind here
	//gameWrapper->HookEvent("Function TAGame.Ball_TA.Explode", std::bind(&StatScraper::YourPluginMethod, this);
// add to onload 
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
	gameWrapper->HookEventWithCallerPost<CarWrapper>("Function TAGame.Car_TA.EventHitBall",
		[this](CarWrapper carWrapper, void* params, std::string eventname) {
			onHitBall(carWrapper, params);
		});

}


void StatScraper::gameTimeTick() {
	// return if the roster is not set yet
	auto roster_sent = cvarManager->getCvar("roster_sent");
	if (!roster_sent.getBoolValue()) { return; }
	ServerWrapper game = gameWrapper->GetOnlineGame();
	if (!game) { return; }
	ArrayWrapper<PriWrapper> priList = game.GetPRIs();
	int player_count = priList.Count();
	LOG("Player Count: {}", player_count);
	if (priList.Count() < 4) { return; }
	json body;
	body["event"] = "player_scores";
	for (int i = 0; i < priList.Count(); i++) {
		PriWrapper pri = priList.Get(i);
		body["player_scores"][i]["player_name"] = pri.GetPlayerName().ToString();
		body["player_scores"][i]["player_score"] = pri.GetMatchScore();
		body["player_scores"][i]["player_goals"] = pri.GetMatchGoals();
		body["player_scores"][i]["player_saves"] = pri.GetMatchSaves();
		body["player_scores"][i]["player_assists"] = pri.GetMatchAssists();
		body["player_scores"][i]["player_shots"] = pri.GetMatchShots();
	}
	sendServerEvent(body);

}
void StatScraper::matchEnded() {
	auto roster_sent = cvarManager->getCvar("roster_sent");
	roster_sent.setValue("0");
}

void StatScraper::startRound() {
	LOG("Got Start Round");
	ServerWrapper game = gameWrapper->GetOnlineGame();
	if (!game) { return; }

	ArrayWrapper<PriWrapper> priList = game.GetPRIs();
	int player_count = priList.Count();
	LOG("Player Count: {}",player_count);
	if (priList.Count() < 4) { return; }

	auto roster_sent = cvarManager->getCvar("roster_sent");
	if (roster_sent.getBoolValue()) { return; }

	roster_sent.setValue("1");
	for (int i = 0; i < priList.Count(); i++) {
		PriWrapper pri = priList.Get(i);
		json body;
		body["event"] = "player_joined";
		body["player_name"] = pri.GetPlayerName().ToString();
		body["player_id"] = std::to_string(pri.GetPlayerID());
		body["player_id_string"] = pri.GetUniqueIdWrapper().GetIdString();
		body["player_platform"] = std::to_string(pri.GetPlatform());
		body["player_team_number"] = pri.GetTeamNum();
		this->sendServerEvent(body);

		LOG("A Player From startRound:");
		LOG("Player Name: {}", pri.GetPlayerName().ToString());
		LOG("Player ID: {}", std::to_string(pri.GetPlayerID()));
		LOG("Player ID String: {}", pri.GetUniqueIdWrapper().GetIdString());
		LOG("Player Platform:{} ", std::to_string(pri.GetPlatform()));
		LOG("Team Num: {}", std::to_string(pri.GetTeamNum()));
	}
}

void StatScraper::onHitBall(CarWrapper car, void* params) {
	auto in_replay = cvarManager->getCvar("in_replay");
	if (in_replay.getBoolValue()) {
		LOG("IN Replay");
		return;
	}
	struct HitBallParams
	{
		uintptr_t Car; //CarWrapper
		uintptr_t Ball; //BallWrapper
		Vector HitLocation;
		Vector HitNormal;
	};
	auto* CastParams = (HitBallParams*)params;
	if (car.IsNull() || !CastParams->Car || !CastParams->Ball) { return; }
	PriWrapper PRI = car.GetPRI();
	if (PRI.IsNull()) { return; }
	this->sendPriStats(PRI, "ball_hit");

	//MMRWrapper mmrw = gameWrapper->GetMMRWrapper();
	//UniqueIDWrapper id = gameWrapper->GetUniqueID();
	//float mmr = mmrw.GetPlayerMMR(id, 11);
	//SkillRank rank = mmrw.GetPlayerRank(id, 11);
	//body["event"] = "MMR";
	//body["mmr"] = mmr;
	//body["rank_div"] = rank.Division;
	//body["rank_tier"] = rank.Tier;
	//body["games_played"] = rank.MatchesPlayed;
}


void StatScraper::sendPriStats(PriWrapper pri, std::string event_name) {
	json body;
	body["event"] = event_name;
	body["player_name"] = pri.GetPlayerName().ToString();
	body["player_score"] = pri.GetMatchScore();
	body["player_goals"] = pri.GetMatchGoals();
	body["player_saves"] = pri.GetMatchSaves();
	body["player_assists"] = pri.GetMatchAssists();
	body["player_shots"] = pri.GetMatchShots();
	this->sendServerEvent(body);
}

void StatScraper::replayStarted() {
	auto in_replay = cvarManager->getCvar("in_replay");
	in_replay.setValue("1");
	json body;
	body["event"] = "replay_started";
	this->sendServerEvent(body);
}

void StatScraper::replayEnded() {
	auto in_replay = cvarManager->getCvar("in_replay");
	in_replay.setValue("0");
	json body;
	body["event"] = "replay_ended";
	this->sendServerEvent(body);
}

void StatScraper::sendServerEvent(json body) {
	CurlRequest req;
	req.url = "http://johns-imac:8822/bakkes";
	req.body = body.dump();

	HttpWrapper::SendCurlJsonRequest(req, [this](int code, std::string result)
		{
			
		});

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
	//LOG(statEvent.GetEventName());
	if (!receiver) { LOG("Null reciever PRI"); return; }
	this->sendPriStats(receiver, statEvent.GetEventName());
	if (statEvent.GetEventName() == "Demolish") {
		LOG("a demolition happened >:(");
		if (!receiver) { LOG("Null reciever PRI"); return; }
		if (!victim) { LOG("Null victim PRI"); return; }

		// Find the primary player's PRI
		PlayerControllerWrapper playerController = gameWrapper->GetPlayerController();
		if (!playerController) { LOG("Null controller"); return; }
		PriWrapper playerPRI = playerController.GetPRI();
		if (!playerPRI) { LOG("Null player PRI"); return; }

		// Compare the primary player to the victim
		if (playerPRI.memory_address != victim.memory_address) {
			LOG("Hah you got demoed get good {}", victim.GetPlayerName().ToString());
			return;
		}
		// Primary player is the victim!
		LOG("I was demoed!!! {} is toxic I'm uninstalling", receiver.GetPlayerName().ToString());
	}
}