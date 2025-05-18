#include "cbase.h"
#include "actual_bullet.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar debug_actual_bullet("debug_actual_bullet", "0", FCVAR_GAMEDLL);

#define BULLET_MODEL "models/weapons/w_bullet.mdl"

LINK_ENTITY_TO_CLASS(actual_bullet, CActualBullet);

BEGIN_DATADESC(CActualBullet)
END_DATADESC()

ConVar sk_bullet_speed_submerged_value("sk_bullet_speed_submerged_value", "6");

void CActualBullet::Precache()
{
	PrecacheModel(BULLET_MODEL);
}

void CActualBullet::Spawn()
{
	SetModel(BULLET_MODEL);
	SetSize(-Vector(1, 1, 1), Vector(1, 1, 1));
}

void CActualBullet::Start(void)
{
	SetThink(&CActualBullet::Think);
	SetNextThink(gpGlobals->curtime);
	SetOwnerEntity(info.m_pAttacker);
}
void CActualBullet::Think(void)
{
	SetNextThink(gpGlobals->curtime + 0.05f);
	Vector vecStart;
	Vector vecEnd;
	float flInterval;

	ConVar* host_timescale = cvar->FindVar("host_timescale");
	float bulletTravelSpeed = m_Speed * host_timescale->GetFloat();

	flInterval = gpGlobals->curtime - GetLastThink();
	vecStart = GetAbsOrigin();
	vecEnd = vecStart + (m_vecDir * (bulletTravelSpeed * flInterval));
	float flDist = (vecStart - vecEnd).Length();

	trace_t tr;
	UTIL_TraceLine(vecStart, vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
	if (debug_actual_bullet.GetBool() == true)
		DebugDrawLine(vecStart, vecEnd, 0, 128, 255, false, 0.05f);

	if (tr.fraction != 1.0)
	{
		FireBulletsInfo_t info2;
		info2.m_iShots = 1;
		info2.m_vecSrc = vecStart;
		info2.m_vecSpread = vec3_origin;
		info2.m_vecDirShooting = m_vecDir;
		info2.m_flDistance = flDist;
		info2.m_iAmmoType = info.m_iAmmoType;
		info2.m_iTracerFreq = 0;
		// BUGFIX: Unhandled exception at 0x24251E78 (server.dll) in crash_hl2.exe_20250321012557_1.dmp: 0xC0000005: Access violation reading location 0x00000000.
		CBaseEntity* pOwner = GetOwnerEntity();
		if (pOwner != NULL)
		{
			pOwner->FireBullets(info2);
		}
		Stop();
	}
	else
	{
		SetAbsOrigin(vecEnd);
	}
}

void CActualBullet::Stop(void)
{
	SetThink(NULL);
	UTIL_Remove(this);
}