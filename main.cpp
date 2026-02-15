#include <windows.h>
#include <mutex>
#include <filesystem>
#include <format>
#include <thread>
#include <toml++/toml.hpp>

#include "nya_dx9_hookbase.h"
#include "nya_commonhooklib.h"
#include "nya_commonmath.h"
#include "nfsuc.h"
#include "chloemenulib.h"

bool bCareerMode = false;
bool bChallengeSeriesMode = false;

#include "util.h"
#include "d3dhook.h"
#include "../MostWantedTimeTrialGhosts/timetrials.h"
#include "hooks/carrender.h"
#include "challengeseries.h"
#include "../MostWantedTimeTrialGhosts/verification.h"

uint32_t PlayerCarType = 0;
ISimable* VehicleConstructHooked(Sim::Param params) {
	DLLDirSetter _setdir;

	auto vehicle = (VehicleParams*)params.mSource;
	if (vehicle->mDriverClass == DRIVER_HUMAN) {
		PlayerCarType = vehicle->mRideInfo->Type;
	}
	if (vehicle->mDriverClass == DRIVER_RACER) {
		static RideInfo info;
		RideInfo::Init(&info, PlayerCarType, CarRenderUsage_AIRacer, 0, 0, 0, 0);
		RideInfo::SetStockParts(&info);

		// copy player car for all opponents
		auto player = GetLocalPlayerVehicle();
		vehicle->mPerformanceMatch = nullptr;
		vehicle->mVehicleAttrib = Attrib::Instance(Attrib::FindCollection(Attrib::StringHash32("pvehicle"), player->GetVehicleKey()), 0);
		vehicle->mRideInfo = &info;
		vehicle->mCustomization = nullptr;
		vehicle->mSkinKey = 0;
		vehicle->mPresetKey = 0;
		vehicle->mVehicleKey = player->GetVehicleKey();
		vehicle->mVehicleResource.mCarType = PlayerCarType;

		// do a config save in every loading screen
		DoConfigSave();
	}
	return PVehicle::Construct(params);
}

auto OnEventFinished_orig = (void(__thiscall*)(GRacerInfo*, GRacerInfo::FinishReason))nullptr;
void __thiscall OnEventFinished(GRacerInfo* a1, GRacerInfo::FinishReason reason) {
	OnEventFinished_orig(a1, reason);

	if (!bChallengeSeriesMode) return;

	bool finished = false;
	if (reason == GRacerInfo::kReason_CrossedFinish) finished = true;
	if (reason == GRacerInfo::kReason_Completed) finished = true;
	if (reason == GRacerInfo::kReason_ChallengeCompleted) finished = true;

	if (finished && a1 == GetRacerInfoFromHandle(GetLocalPlayerSimable())) {
		DLLDirSetter _setdir;
		OnFinishRace();

		// do a config save when finishing a race
		DoConfigSave();
	}
}

auto OnRestartRace_orig = (void(__thiscall*)(void*))nullptr;
void __thiscall OnRestartRace(void* a1) {
	OnRestartRace_orig(a1);
	OnRaceRestart();
}

void MainLoop() {
	static double simTime = -1;

	if (!Sim::Exists() || Sim::GetState() != Sim::STATE_ACTIVE) {
		simTime = -1;
		return;
	}
	if (simTime == Sim::GetTime()) return;
	simTime = Sim::GetTime();

	DLLDirSetter _setdir;
	TimeTrialLoop();

	if (pSelectedChallengeSeriesEvent && GRaceStatus::fObj && !GRaceStatus::fObj->mRaceParms && GRaceParameters::GetEventID(GRaceStatus::fObj->mRaceParms) != pSelectedChallengeSeriesEvent->sEventName) {
		bChallengeSeriesMode = false;
	}

	auto cars = GetActiveVehicles();
	for (auto& car : cars) {
		car->SetDraftZoneModifier(0.0);
	}

	for (auto& skill : GMW2Game::mObj->mRewardModifiers) {
		skill = 0.25;
	}

	auto profile = Database::UserProfile::spUserProfiles[0];
	profile->mOptionsSettings.TheGameplaySettings.JumpCam = false;
	profile->mOptionsSettings.TheGameplaySettings.HighlightCam = 0;

	if (pEventToStart) {
		pEventToStart->SetupEvent();
		pEventToStart = nullptr;
	}
}

void RenderLoop() {
	VerifyTimers();
	TimeTrialRenderLoop();
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			if (NyaHookLib::GetEntryPoint() != 0x4AEC55) {
				MessageBoxA(nullptr, "Unsupported game version! Make sure you're using v1.0.0.1 (.exe size of 10584064 or 10589456 bytes)", "nya?!~", MB_ICONERROR);
				return TRUE;
			}

			gDLLPath = std::filesystem::current_path();
			GetCurrentDirectoryW(MAX_PATH, gDLLDir);

			ChloeMenuLib::RegisterMenu("Time Trial Ghosts", &DebugMenu);

			NyaHooks::SimServiceHook::Init();
			NyaHooks::SimServiceHook::aFunctions.push_back(MainLoop);
			NyaHooks::LateInitHook::Init();
			NyaHooks::LateInitHook::aPreFunctions.push_back(FileIntegrity::VerifyGameFiles);
			NyaHooks::LateInitHook::aFunctions.push_back([]() {
				NyaHooks::PlaceD3DHooks();
				//NyaHooks::WorldServiceHook::Init();
				//NyaHooks::WorldServiceHook::aPreFunctions.push_back(CollectPlayerPos);
				//NyaHooks::WorldServiceHook::aPostFunctions.push_back(CheckPlayerPos);
				NyaHooks::D3DEndSceneHook::aFunctions.push_back(D3DHookMain);
				NyaHooks::D3DResetHook::aFunctions.push_back(OnD3DReset);

				NyaHookLib::Patch<double>(0xBE4208, 1.0 / 120.0); // set sim framerate
				NyaHookLib::Patch<float>(0xC23F94, 1.0 / 120.0); // set sim max framerate

				ApplyVerificationPatches();
				gUndercoverModData.Check();

				*(void**)0xDE6F30 = (void*)&VehicleConstructHooked;
				if (GetModuleHandleA("ZMenuUndercover.asi") || GetModuleHandleA("assimp-vc141-mt.dll")) {
					MessageBoxA(nullptr, "Potential unfair advantage detected! Please remove ZMenuUndercover from your game before using this mod.", "nya?!~", MB_ICONERROR);
					exit(0);
				}
			});

			OnEventFinished_orig = (void(__thiscall*)(GRacerInfo*, GRacerInfo::FinishReason))(*(uint32_t*)(0x6651F1));
			NyaHookLib::Patch(0x6651F1, &OnEventFinished);
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x662456, &OnEventFinished);
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x62DB3B, &OnEventFinished);

			OnRestartRace_orig = (void(__thiscall*)(void*))NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x563D6B, &OnRestartRace);
			NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x5A6EFA, &OnRestartRace);

			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x4553C7, 0x4553D9); // disable traffic

			NyaHookLib::Patch<uint8_t>(0x68542D, 0xEB); // disable jump cam

			// disable drafting
			NyaHookLib::Patch<uint8_t>(0x6FD1B8, 0xEB);
			NyaHookLib::Patch<uint8_t>(0x73E9FC, 0xEB);
			NyaHookLib::Patch<uint8_t>(0x734694, 0xEB);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x72ADB4, 0x72AF09);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x740BF1, 0x740E89);
			NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x72AF82, 0x72B11D);

			SetRacerAIEnabled(false);

			ApplyCarRenderHooks();

			ApplyChallengeSeriesHooks();

			DoConfigLoad();

			WriteLog("Mod initialized");
		} break;
		default:
			break;
	}
	return TRUE;
}