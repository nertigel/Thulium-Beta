#include "../includes.h"

#include "../UTILS/interfaces.h"
#include "../SDK/CClientEntityList.h"
#include "../SDK/CBaseEntity.h"
#include "../SDK/CBaseWeapon.h"
#include "../SDK/IEngine.h"
#include "../SDK/SurfaceData.h"
#include "../SDK/CTrace.h"

#include "../FEATURES/AutoWall.h"

static bool is_autowalling = false;

CAutoWall::Autowall_Return_Info CAutoWall::CalculateDamage(Vector start, Vector end, SDK::CBaseEntity* from_entity, SDK::CBaseEntity* to_entity)
{
	// default values for return info, in case we need to return abruptly
	Autowall_Return_Info return_info;
	return_info.damage = -1;
	return_info.hitgroup = -1;
	return_info.penetration_count = 4;
	return_info.thickness = 0.f;
	return_info.hit_entity = nullptr;

	Autowall_Info autowall_info;
	autowall_info.penetration_count = 4;
	autowall_info.start = start;
	autowall_info.end = end;
	autowall_info.current_position = start;
	return_info.thickness = 0.f;

	// direction 
	MATH::AngleVectors(UTILS::CalcAngle(start, end), &autowall_info.direction);

	// attacking entity
	if (!from_entity)
		from_entity = INTERFACES::ClientEntityList->GetClientEntity(INTERFACES::Engine->GetLocalPlayer());
	if (!from_entity)
		return return_info;

	if (to_entity->GetIsDormant())
		return return_info;

	// setup filters
	auto filter_player = SDK::CTraceFilter();
	filter_player.pSkip1 = from_entity;
	autowall_info.filter = &filter_player;

	// weapon
	auto weapon = reinterpret_cast<SDK::CBaseWeapon*>(INTERFACES::ClientEntityList->GetClientEntity(from_entity->GetActiveWeaponIndex()));
	if (!weapon)
		return return_info;

	// weapon data
	auto weapon_info = weapon->get_full_info();
	if (weapon_info == NULL)
		return return_info;

	if (weapon_info == nullptr)
		return return_info;

	if (!weapon_info)
		return return_info;

	autowall_info.current_damage = weapon_info->iDamage;
	const float range = min(weapon_info->flRange, (start - end).Length());
	end = start + (autowall_info.direction * range);


	while (autowall_info.current_damage > 0 && autowall_info.penetration_count > 0)
	{
		return_info.penetration_count = autowall_info.penetration_count;

		UTIL_TraceLine(autowall_info.current_position, end, MASK_SHOT | CONTENTS_GRATE, from_entity, 0, &autowall_info.enter_trace);
		UTIL_ClipTraceToPlayers(autowall_info.current_position, autowall_info.current_position + autowall_info.direction * 40.f, MASK_SHOT | CONTENTS_GRATE, autowall_info.filter, &autowall_info.enter_trace);

		const float distance_traced = (autowall_info.enter_trace.end - start).Length();
		autowall_info.current_damage *= pow(weapon_info->flRangeModifier, (distance_traced / 500.f));

		// if reached the end
		if (autowall_info.enter_trace.flFraction == 1.f)
		{
			return_info.damage = autowall_info.current_damage;
			return_info.hitgroup = -1;
			return_info.end = autowall_info.enter_trace.end;
			return_info.hit_entity = nullptr;
		}
		// if hit an entity
		if (autowall_info.enter_trace.hitGroup > 0 && autowall_info.enter_trace.hitGroup <= 7 && autowall_info.enter_trace.m_pEnt)
		{
			// checkles gg
			if ((to_entity && autowall_info.enter_trace.m_pEnt != to_entity) ||
				(autowall_info.enter_trace.m_pEnt->GetTeam() == from_entity->GetTeam()))
			{
				return_info.damage = -1;
				return return_info;
			}

			ScaleDamage(autowall_info.enter_trace.m_pEnt, weapon_info, autowall_info.enter_trace.hitGroup, autowall_info.current_damage);

			// fill the return info
			return_info.damage = autowall_info.current_damage;
			return_info.hitgroup = autowall_info.enter_trace.hitGroup;
			return_info.end = autowall_info.enter_trace.end;
			return_info.hit_entity = autowall_info.enter_trace.m_pEnt;

			break;
		}

		// break out of the loop retard
		if (!CanPenetrate(from_entity, autowall_info, weapon_info))
			break;

	}

	return return_info; 
}
bool CAutoWall::CanPenetrate(SDK::CBaseEntity* attacker, Autowall_Info &info, SDK::CSWeaponInfo* weapon_data)
{
	typedef bool(__thiscall* HandleBulletPenetrationFn)(SDK::CBaseEntity*, float&, int&, int*, SDK::trace_t*, Vector*, float, float, float, int, int, float, int*, Vector*, float, float, float*);
	static HandleBulletPenetrationFn HBP = reinterpret_cast<HandleBulletPenetrationFn>(UTILS::FindPattern("client_panorama.dll",
		(PBYTE)"\x53\x8B\xDC\x83\xEC\x08\x83\xE4\xF8\x83\xC4\x04\x55\x8B\x6B\x04\x89\x6C\x24\x04\x8B\xEC\x83\xEC\x78\x8B\x53\x14",
		"xxxxxxxxxxxxxxxxxxxxxxxxxxxx"));

	auto enter_surface_data = INTERFACES::PhysicsProps->GetSurfaceData(info.enter_trace.surface.surfaceProps);
	if (!enter_surface_data)
		return true;

	int use_static_values = 0;
	int material = enter_surface_data->game.material;
	int mask = /*GetWeaponID(local_player) == weapon_taser ? 0x1100 : */0x1002;

	// glass and shit gg
	if (info.enter_trace.m_pEnt && !strcmp("CBreakableSurface",
		info.enter_trace.m_pEnt->GetClientClass()->m_pNetworkName))
		*reinterpret_cast<byte*>(uintptr_t(info.enter_trace.m_pEnt + 0x27C)) = 2;

	is_autowalling = true;
	bool return_value = !HBP(attacker, weapon_data->flPenetration, material, &use_static_values, &info.enter_trace, &info.direction, 0.f, enter_surface_data->game.flPenetrationModifier, enter_surface_data->game.flDamageModifier, 0, mask, weapon_data->flPenetration, &info.penetration_count, &info.current_position, 0.f, 0.f, &info.current_damage);
	is_autowalling = false;
	return return_value;
}
void CAutoWall::ScaleDamage(SDK::CBaseEntity* entity, SDK::CSWeaponInfo* weapon_info, int hitgroup, float& current_damage)
{
	//Cred. to N0xius for reversing this.
	//TODO: _xAE^; look into reversing this yourself sometime

	bool hasHeavyArmor = false;
	int armorValue = entity->GetArmor();

	//Fuck making a new function, lambda beste. ~ Does the person have armor on for the hitbox checked?
	auto IsArmored = [&entity, &hitgroup]()-> bool
	{
		SDK::CBaseEntity* targetEntity = entity;
		switch (hitgroup)
		{
		case HITGROUP_HEAD:
			return targetEntity->HasHelmet();
		case HITGROUP_GENERIC:
		case HITGROUP_CHEST:
		case HITGROUP_STOMACH:
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
			return true;
		default:
			return false;
		}
	};

	switch (hitgroup)
	{
	case HITGROUP_HEAD:
		current_damage *= hasHeavyArmor ? 2.f : 4.f; //Heavy Armor does 1/2 damage
		break;
	case HITGROUP_STOMACH:
		current_damage *= 1.25f;
		break;
	case HITGROUP_LEFTLEG:
	case HITGROUP_RIGHTLEG:
		current_damage *= 0.75f;
		break;
	default:
		break;
	}

	if (armorValue > 0 && IsArmored())
	{
		float bonusValue = 1.f, armorBonusRatio = 0.5f, armorRatio = weapon_info->flArmorRatio / 2.f;

		//Damage gets modified for heavy armor users
		if (hasHeavyArmor)
		{
			armorBonusRatio = 0.33f;
			armorRatio *= 0.5f;
			bonusValue = 0.33f;
		}

		auto NewDamage = current_damage * armorRatio;

		if (hasHeavyArmor)
			NewDamage *= 0.85f;

		if (((current_damage - (current_damage * armorRatio)) * (bonusValue * armorBonusRatio)) > armorValue)
			NewDamage = current_damage - (armorValue / armorBonusRatio);

		current_damage = NewDamage;
	}
}
bool CAutoWall::IsArmored(SDK::CBaseEntity *player, int armorVal, int hitgroup)
{
	bool res = false;

	if (armorVal > 0)
	{
		switch (hitgroup)
		{
		case HITGROUP_GENERIC:
		case HITGROUP_CHEST:
		case HITGROUP_STOMACH:
		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:

			res = true;
			break;

		case HITGROUP_HEAD:

			res = player->HasHelmet();
			break;

		}
	}

	return res;
}

bool CAutoWall::FloatingPointIsVisible(SDK::CBaseEntity* local_player, const Vector &point)
{
	SDK::trace_t Trace;
	Vector end = point;

	Vector local_position = local_player->GetVecOrigin() + local_player->GetViewOffset();
	UTIL_TraceLineSig(local_position, end, MASK_SOLID, local_player, 0, &Trace);

	if (Trace.flFraction == 1.0f)
	{
		return true;
	}

	return false;
}
CAutoWall* autowall = new CAutoWall();