bool HideGhostCar(NyaMat4x4* carMatrix) {
	auto car = GetClosestActiveVehicle(RenderToWorldCoords(carMatrix->p));
	if (!car) return false;
	if (car == GetLocalPlayerVehicle()) return false;
	if (car->IsStaging()) return false;
	if (car->GetDriverClass() == DRIVER_TRAFFIC || car->GetDriverClass() == DRIVER_COP) return false;

	bool hide = nGhostVisuals == GHOST_HIDE;
	if (!hide) {
		auto playerCar = GetLocalPlayerVehicle();
		if (!playerCar) return false;

		auto dist = (*playerCar->GetPosition() - *car->GetPosition()).length();
		if (dist < 8) hide = true;
	}
	return hide;
}

class eView;
auto CarGetVisibleStateOrig = (int(__thiscall*)(eView*, const bVector3*, const bVector3*, bMatrix4*))nullptr;
int __thiscall CarGetVisibleStateHooked(eView* a1, const bVector3* a2, const bVector3* a3, bMatrix4* a4) {
	auto carMatrix = (NyaMat4x4*)a4;
	if (TheGameFlowManager.CurrentGameFlowState == GAMEFLOW_STATE_RACING && !IsInNIS() && (nGhostVisuals == GHOST_HIDE || nGhostVisuals == GHOST_HIDE_NEARBY)) {
		if (HideGhostCar(carMatrix)) {
			return 0;
		}
	}
	return CarGetVisibleStateOrig(a1, a2, a3, a4);
}

void ApplyCarRenderHooks() {
	CarGetVisibleStateOrig = (int(__thiscall*)(eView*, const bVector3*, const bVector3*, bMatrix4*))NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x8786DA, &CarGetVisibleStateHooked);
}