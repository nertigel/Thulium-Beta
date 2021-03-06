#pragma once

namespace SDK
{
	class CUserCmd;
	class CBaseEntity;
	struct Weapon_Info;
	class CSWeaponInfo;
}

class CAutoWall
{
private:
	struct Autowall_Return_Info
	{
		int damage;
		int hitgroup;
		int penetration_count;
		float thickness;
		Vector end;
		SDK::CBaseEntity* hit_entity;
	};
	void UTIL_TraceLineSig(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, const SDK::CBaseEntity *ignore, int collisionGroup, SDK::trace_t *ptr)
	{
		typedef int(__fastcall* UTIL_TraceLine_t)(const Vector&, const Vector&, unsigned int, const SDK::CBaseEntity*, int, SDK::trace_t*);
		static UTIL_TraceLine_t TraceLine = (UTIL_TraceLine_t)UTILS::FindSignature("client_panorama.dll", "55 8B EC 83 E4 F0 83 EC 7C 56 52");
		TraceLine(vecAbsStart, vecAbsEnd, mask, ignore, collisionGroup, ptr);
	}
	void UTIL_ClipTraceToPlayers(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, SDK::ITraceFilter* filter, SDK::trace_t* tr)
	{
		static DWORD dwAddress = UTILS::FindPattern("client_panorama.dll", (BYTE*)"\x53\x8B\xDC\x83\xEC\x08\x83\xE4\xF0\x83\xC4\x04\x55\x8B\x6B\x04\x89\x6C\x24\x04\x8B\xEC\x81\xEC\x00\x00\x00\x00\x8B\x43\x10", "xxxxxxxxxxxxxxxxxxxxxxxx????xxx");
		if (!dwAddress)
			return;

		_asm
		{
			MOV        EAX, filter
			LEA        ECX, tr
			PUSH    ECX
			PUSH    EAX
			PUSH    mask
			LEA        EDX, vecAbsEnd
			LEA        ECX, vecAbsStart
			CALL    dwAddress
			ADD        ESP, 0xC
		}
	}
public:
	struct Autowall_Info
	{
		Vector start;
		Vector end;
		Vector current_position;

		Vector direction;

		SDK::ITraceFilter* filter;
		SDK::trace_t enter_trace;

		float thickness;
		float current_damage;
		int penetration_count;
	};
	Autowall_Return_Info CalculateDamage(Vector start, Vector end, SDK::CBaseEntity * from_entity, SDK::CBaseEntity * to_entity);
	bool CanPenetrate(SDK::CBaseEntity * attacker, Autowall_Info & info, SDK::CSWeaponInfo * weapon_data);
	void ScaleDamage(SDK::CBaseEntity* entity, SDK::CSWeaponInfo* weapon_info, int hitgroup, float& current_damage);
	bool IsArmored(SDK::CBaseEntity* Entity, int ArmorValue, int Hitgroup);
	bool FloatingPointIsVisible(SDK::CBaseEntity * local_player, const Vector & point);

	void UTIL_TraceLine(Vector& vecAbsStart, Vector& vecAbsEnd, unsigned int mask, SDK::CBaseEntity *ignore, int collisionGroup, SDK::trace_t *ptr)
	{
		SDK::Ray_t ray;

		SDK::CTraceFilter filter;
		filter.pSkip1 = ignore;

		ray.Init(vecAbsStart, vecAbsEnd);

		INTERFACES::Trace->TraceRay(ray, mask, &filter, ptr);
	}
};

extern CAutoWall* autowall;