int GetNumRacesHooked() {
	return bChallengeSeriesMode ? 1 : 0;
}

uint32_t nCurrentEventKey;
void* __thiscall GetRaceKeyHooked(Attrib::Instance* instance, uint32_t attributeKey, uint32_t index) {
	static uint32_t key;
	key = nCurrentEventKey;
	return &key;
}

class ChallengeSeriesEvent;
ChallengeSeriesEvent* pSelectedChallengeSeriesEvent = nullptr;
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

	std::string GetCarNameForGhost() const {
		//if (!Attrib::FindCollection(Attrib::StringHash32("presetride"), Attrib::StringHash32(sCarPreset.c_str()))) return sCarPreset;
		//auto preset = Attrib::Gen::presetride(Attrib::StringHash32(sCarPreset.c_str()), 0);
		//auto pvehicle = Attrib::Gen::pvehicle(preset.GetLayout()->PresetCar.hash, 0);
		//return pvehicle.GetLayout()->CollectionName;

		auto preset = Attrib::FindCollection(Attrib::StringHash32("presetride"), Attrib::StringHash32(sCarPreset.c_str()));
		if (!preset) return sCarPreset;

		auto a = (uint32_t*)Attrib::Collection::GetData(preset, 0xF833C06F, 0);
		auto carHash = a[1];
		auto pvehicle = Attrib::Gen::pvehicle(carHash, 0);
		return pvehicle.GetLayout()->CollectionName;

		//auto pvehicle = Attrib::FindCollection(Attrib::StringHash32("pvehicle"), carHash);
		//if (!pvehicle) return sCarPreset;
		//return pvehicle->
	}

	tReplayGhost GetPBGhost() {
		while (bPBGhostLoading) { Sleep(0); }

		if (PBGhost.nFinishTime != 0) return PBGhost;

		bPBGhostLoading = true;
		tReplayGhost temp;
		auto bak = bChallengeSeriesMode;
		bChallengeSeriesMode = true;
		LoadPB(&temp, GetCarNameForGhost(), sEventName, GetLapCount(), 0, nullptr);
		bChallengeSeriesMode = bak;
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
		auto bak = bChallengeSeriesMode;
		bChallengeSeriesMode = true;
		auto times = CollectReplayGhosts(GetCarNameForGhost(), sEventName, GetLapCount(), nullptr);
		bChallengeSeriesMode = bak;
		if (!times.empty()) {
			times[0].aTicks.clear(); // just in case
			targetTime = aTargetGhosts[nDifficulty] = times[0];
		}
		nNumGhosts[nDifficulty] = times.size();
		bTargetGhostLoading = false;
		return targetTime;
	}

	void SetupEvent() {
		if (TheGameFlowManager.CurrentGameFlowState != GAMEFLOW_STATE_RACING) return;
		if (IsInNIS()) return;

		bChallengeSeriesMode = true;

		auto race = GRaceDatabase::GetRaceFromHash(GRaceDatabase::mObj, Attrib::StringHash32(sEventName.c_str()));
		nCurrentEventKey = race->GetCollectionKey();

		pSelectedChallengeSeriesEvent = this;

#ifdef RACESTART_HUB
		auto eventKey = race->GetCollectionKey();

		race->mIndex->mNumLaps = GRaceParameters::GetIsLoopingRace(race) ? GetLapCount() : 1;
		race->mIndex->mPlayerCarTypeHash = Attrib::StringHash32(sCarPreset.c_str());
		race->mIndex->mFlags |= GRaceIndexData::kFlag_UsePresetRide;
		race->BlockUntilLoaded();

		if (auto addr = (char**)Attrib::Instance::GetAttributePointer(race->mRaceRecord, Attrib::StringHash32("IntroMovie"), 0)) {
			*addr = nullptr;
		}

		if (auto addr = (int*)Attrib::Instance::GetAttributePointer(race->mRaceRecord, Attrib::StringHash32("NumLaps"), 0)) {
			*addr = GetLapCount();
		}

		IGameStatus::mInstance->SetRoaming();
		GEvent::Cleanup(GEvent::sInstance);
		GHub::StartEventFromKey(GHub::sCurrentHub, eventKey);
		GHub::sCurrentHub->mCurrentEventKey = eventKey;
		GHub::sCurrentHub->mCurrentEventHash = race->GetEventHash();
		GHub::sCurrentHub->mRequestedEventKey = eventKey;
		GHub::sCurrentHub->mSelectedEventIndex = 0;
		GHub::sCurrentHub->mCurrentEventType = race->GetEventType();
		GHub::StartCurrentEvent(GHub::sCurrentHub);
		GEvent::Prepare(GEvent::sInstance);
		IGameStatus::mInstance->EventLaunched();
#elif RACESTART_CUSTOMRACE
		auto custom = GRaceDatabase::AllocCustomRace(GRaceDatabase::mObj, race);
		custom->mIndex->mNumLaps = GRaceParameters::GetIsLoopingRace(race) ? GetLapCount() : 1;
		custom->mIndex->mPlayerCarTypeHash = Attrib::StringHash32(sCarPreset.c_str());
		custom->mIndex->mFlags |= GRaceIndexData::kFlag_UsePresetRide;

		IGameStatus::mInstance->SetRoaming();
		GEvent::Cleanup(GEvent::sInstance);
		if (!custom->GetActivity()) {
			GRaceCustom::CreateRaceActivity(custom);
		}
		if (custom->GetActivity()) {
			GManager::ConnectRuntimeInstances(GManager::mObj);
			GEvent::sInstance->mEventActivity = custom->GetActivity();
			GEvent::Prepare(GEvent::sInstance);
			GManager::StartRaceActivityFromInGame(GManager::mObj, custom);
			IGameStatus::mInstance->EventLaunched();
		}
#else
		static_assert(0);
#endif

		//IGameStatus::mInstance->SetRoaming();
		//IGameStatus::mInstance->ToggleShortMissionIntroNIS(true);
		//GHub::StartEventFromKey(GHub::sCurrentHub, Attrib::StringHash32(sEventName.c_str()));
		//GHub::StartCurrentEvent(GHub::sCurrentHub);
		//IGameStatus::mInstance->EventLaunched();
	}
};

std::vector<ChallengeSeriesEvent> aVanillaChallengeSeries = {
	ChallengeSeriesEvent("E007", "ce_240sx"),
	ChallengeSeriesEvent("E650", "racer_170"),
	ChallengeSeriesEvent("E013", "zack"),
	ChallengeSeriesEvent("E008", "diecast_911gt2"),
	ChallengeSeriesEvent("E136", "m09"),
	ChallengeSeriesEvent("E009", "pinkslip_racer_005"),
	ChallengeSeriesEvent("E017", "chase"),
	ChallengeSeriesEvent("E203", "pinkslip_racer_020"),
	ChallengeSeriesEvent("E124", "nickel", 1),
	ChallengeSeriesEvent("E260", "nfsdotcom"),
	ChallengeSeriesEvent("E374", "gmac"),
	ChallengeSeriesEvent("E501", "ce_ccx", 1),
	ChallengeSeriesEvent("E298", "pinkslip_racer_024", 2),
	ChallengeSeriesEvent("E654", "rose"),
	ChallengeSeriesEvent("E016", "sedan", 1),
};

std::vector<ChallengeSeriesEvent> aReformedChallengeSeries = {
	ChallengeSeriesEvent("E007", "starter_eclipse"),
	ChallengeSeriesEvent("E650", "e470_pinkslip"),
	ChallengeSeriesEvent("E013", "bonus_zack"),
	ChallengeSeriesEvent("E008", "bonus_cs_gt3"),
	ChallengeSeriesEvent("E136", "bonus_evox"),
	ChallengeSeriesEvent("E009", "e013_pinkslip2"),
	ChallengeSeriesEvent("E104", "bonus_dominator"),
	ChallengeSeriesEvent("E203", "e124_pinkslip2"),
	ChallengeSeriesEvent("E124", "bonus_nickel", 1),
	ChallengeSeriesEvent("E260", "m30"),
	ChallengeSeriesEvent("E374", "m33"),
	ChallengeSeriesEvent("E501", "bonus_rose", 1),
	ChallengeSeriesEvent("E298", "m28", 2),
	ChallengeSeriesEvent("E654", "bonus_370z"),
	//ChallengeSeriesEvent("E112", ""),
	ChallengeSeriesEvent("E016", "sedan", 2),
	// e124_pinkslip4 supra
	// starter_civic
};
auto aNewChallengeSeries = &aVanillaChallengeSeries;

ChallengeSeriesEvent* GetChallengeEvent(uint32_t hash) {
	for (auto& event : *aNewChallengeSeries) {
		if (!GRaceDatabase::GetRaceFromHash(GRaceDatabase::mObj, Attrib::StringHash32(event.sEventName.c_str()))) {
			MessageBoxA(0, std::format("Failed to find event {}", event.sEventName).c_str(), "nya?!~", MB_ICONERROR);
			exit(0);
		}
	}
	for (auto& event : *aNewChallengeSeries) {
		if (Attrib::StringHash32(event.sEventName.c_str()) == hash) return &event;
	}
	return nullptr;
}

ChallengeSeriesEvent* GetChallengeEvent(const std::string& str) {
	for (auto& event : *aNewChallengeSeries) {
		if (event.sEventName == str) return &event;
	}
	return nullptr;
}

void OnChallengeSeriesEventPB() {
	pSelectedChallengeSeriesEvent->ClearPBGhost();
}

ChallengeSeriesEvent* pEventToStart = nullptr;
void ChallengeSeriesMenu() {
	if (gUndercoverModData.bReformedInstalled) {
		aNewChallengeSeries = &aReformedChallengeSeries;
	}

	for (auto& event : *aNewChallengeSeries) {
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

const char* __thiscall GetPresetRideHooked(GRaceParameters* pThis) {
	auto event = GetChallengeEvent(pThis->GetEventID(pThis));
	if (!event) return nullptr;
	return event->sCarPreset.c_str();
}

void ApplyChallengeSeriesHooks() {
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x6424DD, &GetNumRacesHooked);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x6424F7, &GetRaceKeyHooked);
	NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x63F550, &GetPresetRideHooked);
}