class ChallengeSeriesEvent {
public:
	std::string sEventName;
	std::string sCarPreset;
	int nLapCountOverride = 0;

	bool bPBGhostLoading = false;
	bool bTargetGhostLoading = false;
	tReplayGhost PBGhost = {};
	tReplayGhost aTargetGhosts[NUM_DIFFICULTY] = {};
	int nNumGhosts[NUM_DIFFICULTY] = {};

	ChallengeSeriesEvent(const char* eventName, const char* carPreset, int lapCount = 0) : sEventName(eventName), sCarPreset(carPreset), nLapCountOverride(lapCount) {}

	GRaceParameters* GetRace() const {
		return GRaceDatabase::GetRaceFromHash(GRaceDatabase::mObj, Attrib::StringHash32(sEventName.c_str()));
	}

	int GetLapCount() {
		if (nLapCountOverride > 0) return nLapCountOverride;
		return nLapCountOverride = GRaceParameters::GetNumLaps(GetRace());
	}

	void ClearPBGhost() {
		PBGhost = {};
	}

	tReplayGhost GetPBGhost() {
		while (bPBGhostLoading) { Sleep(0); }

		if (PBGhost.nFinishTime != 0) return PBGhost;

		bPBGhostLoading = true;
		tReplayGhost temp;
		LoadPB(&temp, sCarPreset, sEventName, GetLapCount(), 0, nullptr);
		temp.aTicks.clear(); // just in case
		PBGhost = temp;
		bPBGhostLoading = false;
		return temp;
	}

	tReplayGhost GetTargetGhost() {
		while (bTargetGhostLoading) { Sleep(0); }

		if (aTargetGhosts[nDifficulty].nFinishTime != 0) return aTargetGhosts[nDifficulty];

		bTargetGhostLoading = true;
		tReplayGhost targetTime;
		auto times = CollectReplayGhosts(sCarPreset, sEventName, GetLapCount(), nullptr);
		if (!times.empty()) {
			times[0].aTicks.clear(); // just in case
			targetTime = aTargetGhosts[nDifficulty] = times[0];
		}
		nNumGhosts[nDifficulty] = times.size();
		bTargetGhostLoading = false;
		return targetTime;
	}

	void SetupEvent() {
		// todo

		/*if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING) return;

		auto race = GRaceDatabase::GetRaceFromHash(GRaceDatabase::mObj, Attrib::StringHash32(sEventName.c_str()));

		RaceParameters::InitWithDefaults(&TheRaceParameters);
		SkipFETrackNumber = TheRaceParameters.TrackNumber = GetTrackID();
		SkipFENumAICars = bChallengesOneGhostOnly ? 1 : 7;
		SkipFERaceID = sEventName.c_str();
		SkipFENumLaps = GRaceParameters::GetIsLoopingRace(race) ? GetLapCount() : 1;
		SkipFEPlayerCar = sCarPreset.c_str();
		if (IsSpeedEvent()) {
			SkipFEPlayerBluePrintType = BLUEPRINT_SPEED_CHALLENGE;
		}
		else if (IsDriftEvent()) {
			SkipFEPlayerBluePrintType = BLUEPRINT_DRIFT;
		}
		else {
			SkipFEPlayerBluePrintType = BLUEPRINT_GRIP;
		}

		GameFlowManager::ReloadTrack(&TheGameFlowManager);*/
	}
};

std::vector<ChallengeSeriesEvent> aNewChallengeSeries = {
	ChallengeSeriesEvent("1.gr.1", "player_d_day", 2),
};

ChallengeSeriesEvent* GetChallengeEvent(uint32_t hash) {
	for (auto& event : aNewChallengeSeries) {
		if (!GRaceDatabase::GetRaceFromHash(GRaceDatabase::mObj, Attrib::StringHash32(event.sEventName.c_str()))) {
			MessageBoxA(0, std::format("Failed to find event {}", event.sEventName).c_str(), "nya?!~", MB_ICONERROR);
			exit(0);
		}
	}
	for (auto& event : aNewChallengeSeries) {
		if (Attrib::StringHash32(event.sEventName.c_str()) == hash) return &event;
	}
	return nullptr;
}

ChallengeSeriesEvent* GetChallengeEvent(const std::string& str) {
	for (auto& event : aNewChallengeSeries) {
		if (event.sEventName == str) return &event;
	}
	return nullptr;
}

void OnChallengeSeriesEventPB() {
	auto event = GetChallengeEvent(GRaceParameters::GetEventID(GRaceStatus::fObj->mRaceParms));
	if (!event) return;
	event->ClearPBGhost();
}

ChallengeSeriesEvent* pEventToStart = nullptr;
void ChallengeSeriesMenu() {
	for (auto& event : aNewChallengeSeries) {
		auto pb = event.GetPBGhost();
		auto target = event.GetTargetGhost();
		auto optionName = event.sEventName; // todo

		auto targetName = GetRealPlayerName(target.sPlayerName);
		auto targetTime = std::format("Target Time - {} ({})", FormatTime(target.nFinishTime), targetName);

		if (pb.nFinishTime != 0) {
			bool won = pb.nFinishTime <= target.nFinishTime;
			if (pb.nFinishPoints && target.nFinishPoints) {
				won = pb.nFinishPoints >= target.nFinishPoints;
			}
			if (won) {
				optionName += std::format(" - Completed");
			}
		}

		if (DrawMenuOption(optionName, targetTime)) {
			ChloeMenuLib::BeginMenu();
			DrawMenuOption(std::format("Event - {}", event.sEventName));
			DrawMenuOption(std::format("Car - {}", event.sCarPreset));
			DrawMenuOption(targetTime);
			if (pb.nFinishTime != 0) {
				DrawMenuOption(std::format("Personal Best - {}", FormatTime(pb.nFinishTime)));
			}
			if (DrawMenuOption("Launch Event")) {
				pEventToStart = &event;
			}
			ChloeMenuLib::EndMenu();
		}
	}
}