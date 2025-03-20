//=============================================================================//
//
// Purpose: An entity that creates NPCs in the game similar to npc_maker's. But this entity
//			spawns enemies in wave-after-wave and handles it's customization trough script files
//			and VScript.
//
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "monstermaker.h"
#include "mapentities.h"
#include "logic_assault.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int const MAX_ENEMIES = 8;

LINK_ENTITY_TO_CLASS(logic_assault, CLogicAssault);

BEGIN_DATADESC(CLogicAssault)
//DEFINE_KEYFIELD(m_ReuseDelay, FIELD_FLOAT, "ReuseDelay"),
//DEFINE_KEYFIELD(m_RenameNPC, FIELD_STRING, "RenameNPC"),
//DEFINE_FIELD(m_TimeNextAvailable, FIELD_TIME),

DEFINE_OUTPUT(m_OnSpawnNPC, "OnSpawnNPC"),
DEFINE_OUTPUT(m_OnFirstWave, "OnFirstWave"),
DEFINE_OUTPUT(m_OnNextWave, "OnNextWave"),
DEFINE_OUTPUT(m_OnWaveDefeated, "OnWaveDefeated"),
DEFINE_OUTPUT(m_OnAllWavesDefeated, "OnAllWaveDefeated"),
END_DATADESC()

void CLogicAssault::Spawn(void)
{
	if (!m_bDisabled)
	{
		SetThink(&CLogicAssault::AssaultThink);
		SetNextThink(gpGlobals->curtime + 0.1f);
	}
	else
	{
		//wait to be activated.
		SetThink(&CBaseNPCMaker::SUB_DoNothing);
	}
}

void CLogicAssault::AssaultThink(void)
{
}

//-----------------------------------------------------------------------------
// A not-very-robust check to see if a human hull could fit at this location.
// used to validate spawn destinations.
//-----------------------------------------------------------------------------
bool CLogicAssault::HumanHullFits(const Vector& vecLocation, CBaseEntity* m_hIgnoreEntity)
{
	trace_t tr;
	UTIL_TraceHull(vecLocation,
		vecLocation + Vector(0, 0, 1),
		NAI_Hull::Mins(HULL_HUMAN),
		NAI_Hull::Maxs(HULL_HUMAN),
		MASK_NPCSOLID,
		m_hIgnoreEntity,
		COLLISION_GROUP_NONE,
		&tr);

	if (tr.fraction == 1.0)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not it is OK to make an NPC at this instant.
//-----------------------------------------------------------------------------
bool CLogicAssault::CanMakeNPC(bool bIgnoreSolidEntities, CBaseEntity* m_hIgnoreEntity)
{
	if (m_iNumEnemies >= MAX_ENEMIES)
	{
		return false;
	}

	Vector mins = GetAbsOrigin() - Vector(34, 34, 0);
	Vector maxs = GetAbsOrigin() + Vector(34, 34, 0);
	maxs.z = GetAbsOrigin().z;

	// If we care about not hitting solid entities, look for 'em
	if (!bIgnoreSolidEntities)
	{
		CBaseEntity* pList[128];

		int count = UTIL_EntitiesInBox(pList, 128, mins, maxs, FL_CLIENT | FL_NPC);
		if (count)
		{
			//Iterate through the list and check the results
			for (int i = 0; i < count; i++)
			{
				//Don't build on top of another entity
				if (pList[i] == NULL)
					continue;

				//If one of the entities is solid, then we may not be able to spawn now
				if ((pList[i]->GetSolidFlags() & FSOLID_NOT_SOLID) == false)
				{
					// Since the outer method doesn't work well around striders on account of their huge bounding box.
					// Find the ground under me and see if a human hull would fit there.
					trace_t tr;
					UTIL_TraceHull(GetAbsOrigin() + Vector(0, 0, 2),
						GetAbsOrigin() - Vector(0, 0, 8192),
						NAI_Hull::Mins(HULL_HUMAN),
						NAI_Hull::Maxs(HULL_HUMAN),
						MASK_NPCSOLID,
						m_hIgnoreEntity,
						COLLISION_GROUP_NONE,
						&tr);

					if (!HumanHullFits(tr.endpos + Vector(0, 0, 1), m_hIgnoreEntity))
					{
						return false;
					}
				}
			}
		}
	}

	// Do we need to check to see if the player's looking?
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBasePlayer* pPlayer = UTIL_PlayerByIndex(i);
		if (pPlayer)
		{
			// Only spawn if the player's looking away from me
			if (pPlayer->FInViewCone(GetAbsOrigin()) && pPlayer->FVisible(GetAbsOrigin()))
			{
				if (!(pPlayer->GetFlags() & FL_NOTARGET))
					return false;
				DevMsg(2, "logic_assault spawning even though seen due to notarget\n");
			}
		}
	}

	return true;
}