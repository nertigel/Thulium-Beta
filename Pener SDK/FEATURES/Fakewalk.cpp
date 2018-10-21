#include "../includes.h"
#include "../UTILS/interfaces.h"
#include "../SDK/IEngine.h"
#include "../SDK/CUserCmd.h"
#include "../SDK/CBaseEntity.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/CTrace.h"
#include "../SDK/CBaseWeapon.h"
#include "../SDK/CGlobalVars.h"
#include "../SDK/NetChannel.h"
#include "../SDK/CBaseAnimState.h"
#include "../SDK/ConVar.h"
#include "../FEATURES/AutoWall.h"
#include "../FEATURES/Fakewalk.h"
#include "../FEATURES/Aimbot.h"

#include <time.h>
#include <iostream>
void CFakewalk::do_fakewalk(SDK::CUserCmd* cmd)
{
	if (UTILS::INPUT::input_handler.GetKeyState(UTILS::INPUT::input_handler.keyBindings(SETTINGS::settings.fakewalk_key)) & 1)
	{
		auto local_player = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
		if (!local_player || local_player->GetHealth() <= 0) return;

		auto net_channel = INTERFACES::Engine->GetNetChannel();
		if (!net_channel) return;

		auto animstate = local_player->GetAnimState();
		if (!animstate) return;

		fake_walk = true;

		if (fabs(local_update - INTERFACES::Globals->curtime) <= 0.05)
		{
			cmd->move.x = 450;
			aimbot->rotate_movement(UTILS::CalcAngle(Vector(0, 0, 0), local_player->GetVelocity()).y + 180.f, cmd);
		}

		static int choked = 0;
		choked = choked > 7 ? 0 : choked + 1;
		cmd->move.x = choked < 2 || choked > 5 ? 0 : cmd->move.x;
		cmd->move.y = choked < 2 || choked > 5 ? 0 : cmd->move.y;
		GLOBAL::should_send_packet = choked < 1;
	}
	else
		fake_walk = false;
}

CFakewalk* fakewalk = new CFakewalk();