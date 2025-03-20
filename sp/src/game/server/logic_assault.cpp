//=============================================================================//
//
// Purpose: An entity that creates NPCs in the game similar to npc_maker's. But this entity
//			spawns enemies in wave-after-wave and handles it's customization trough script files
//			and VScript.
//			Most of this code is from monstermaker.cpp but repurposed
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
int const MAX_ENEMIES_PER_WAVE = 4;

LINK_ENTITY_TO_CLASS(logic_assault, CLogicAssault);

BEGIN_DATADESC(CLogicAssault)
//DEFINE_KEYFIELD(m_ReuseDelay, FIELD_FLOAT, "ReuseDelay"),
//DEFINE_KEYFIELD(m_RenameNPC, FIELD_STRING, "RenameNPC"),
//DEFINE_FIELD(m_TimeNextAvailable, FIELD_TIME),

DEFINE_OUTPUT(m_OnSpawnNPC, "OnSpawnNPC"),
DEFINE_OUTPUT(m_OnFirstPhase, "OnFirstPhase"),
DEFINE_OUTPUT(m_OnNextPhase, "OnNextPhase"),
DEFINE_OUTPUT(m_OnWaveDefeated, "OnWaveDefeated"),
DEFINE_OUTPUT(m_OnAllWavesDefeated, "OnAllWaveDefeated"),
END_DATADESC()

void CLogicAssault::Spawn(void)
{
	m_iNumWave = 1;
	m_iPhase = 1;
	if (!m_bDisabled)
	{
		SetThink(&CLogicAssault::AssaultThink);
		SetNextThink(gpGlobals->curtime + 0.1f);
	}
	else
	{
		//wait to be activated.
		SetThink(&CLogicAssault::SUB_DoNothing);
	}
}

void CLogicAssault::AssaultThink(void)
{
	SetNextThink(gpGlobals->curtime + 0.1f);
	if (m_iNumWave == 1 && !m_bIsFirstWaveTriggered)
	{
		m_bIsFirstWaveTriggered = true;
		m_OnFirstPhase.FireOutput(this, this);
	}
	MakeNPC();
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

CNPCSpawnDestination* CLogicAssault::GetNearbySpawnPoint()
{
	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
	if (pPlayer == NULL) return NULL;
	CBaseEntity* pSpawnPoint = gEntList.FindEntityByClassnameNearest("info_npc_spawn_destination", pPlayer->GetAbsOrigin(), 300.0F);
	if (pSpawnPoint && pSpawnPoint->NameMatches("info_enemy_spawn_point"))
	{
		return dynamic_cast<CNPCSpawnDestination*>(pSpawnPoint);
	}
}

void CLogicAssault::DeathNotice(CBaseEntity* pVictim)
{
	// ok, we've gotten the deathnotice from our child, now clear out its owner if we don't want it to fade.
	m_iNumEnemies--;

	// If we're here, we're getting erroneous death messages from children we haven't created
	AssertMsg(m_iNumEnemies >= 0, "logic_assault receiving child death notice but thinks has no children\n");

	// If all enemies in this wave are defeated or there is one member left, proceed to the next wave
	if (m_iNumEnemies <= 1)
	{
		// If we beat all three phases, go to the next wave
		if (m_iPhase > 3)
		{
			m_iPhase = 0;
			m_iNumWave++;
			m_OnWaveDefeated.FireOutput(this, this);
		}
		else
		{
			m_iPhase++;
			m_OnNextPhase.FireOutput(this, this);
		}
		if (!m_bEndlessWaves && m_iNumWave > m_iMaxWaves)
		{
			m_OnAllWavesDefeated.FireOutput(this, this);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Creates the NPC.
//-----------------------------------------------------------------------------
void CLogicAssault::MakeNPC(void)
{
	if (!CanMakeNPC())
		return;

	CNPCSpawnDestination* pSpawnPoint = GetNearbySpawnPoint();
	if (pSpawnPoint == NULL)
		return;

	CAI_BaseNPC* pent = (CAI_BaseNPC*)CreateEntityByName("npc_combine_s");

	if (!pent)
	{
		Warning("NULL Ent in NPCMaker!\n");
		return;
	}

	m_OnSpawnNPC.Set(pent, pent, this);

	pent->SetAbsOrigin(pSpawnPoint->GetAbsOrigin());

	// Strip pitch and roll from the spawner's angles. Pass only yaw to the spawned NPC.
	QAngle angles = pSpawnPoint->GetAbsAngles();
	angles.x = 0.0;
	angles.z = 0.0;
	pent->SetAbsAngles(angles);

	pent->AddSpawnFlags(SF_NPC_FALL_TO_GROUND);
	pent->AddSpawnFlags(SF_NPC_FADE_CORPSE);

	if (pent->ClassMatches("npc_combine_s"))
	{
		if (random->RandomFloat() < 0.25F)
		{
			pent->m_spawnEquipment = AllocPooledString("weapon_shotgun");
		}
		else if (random->RandomFloat() < 0.15F)
		{
			pent->m_spawnEquipment = AllocPooledString("weapon_ar2");
		}
		else
		{
			pent->m_spawnEquipment = AllocPooledString("weapon_smg1");
		}
	}
	else if (pent->ClassMatches("npc_metropolice"))
	{
		if (random->RandomFloat() < 0.25F)
		{
			pent->m_spawnEquipment = AllocPooledString("weapon_shotgun");
		}
		else
		{
			pent->m_spawnEquipment = AllocPooledString("weapon_smg1");
		}
	}
	else if (pent->ClassMatches("npc_citizen"))
	{
		if (random->RandomFloat() < 0.25F)
		{
			pent->m_spawnEquipment = AllocPooledString("weapon_shotgun");
		}
		else
		{
			pent->m_spawnEquipment = AllocPooledString("weapon_smg1");
		}
	}
	pent->SetSquadName(AllocPooledString("squad_assault_group"));
	pent->SetHintGroup(AllocPooledString("hint_assault"));

	DispatchSpawn(pent);
	pent->SetOwnerEntity(this);
	ChildPostSpawn(pent);

	m_iNumEnemies++;// count this NPC
}

void CLogicAssault::Enable(void)
{
	m_bDisabled = false;
	if (m_iNumWave == 0)
	{
		m_iNumWave = 1;
	}
	SetThink(&CLogicAssault::AssaultThink);
	SetNextThink(gpGlobals->curtime);
}

void CLogicAssault::Disable(void)
{
	m_bDisabled = true;
	SetThink(NULL);
}

// Taken from monstermaker.cpp
void CLogicAssault::ChildPostSpawn(CAI_BaseNPC* pChild)
{
	// If I'm stuck inside any props, remove them
	bool bFound = true;
	while (bFound)
	{
		trace_t tr;
		UTIL_TraceHull(pChild->GetAbsOrigin(), pChild->GetAbsOrigin(), pChild->WorldAlignMins(), pChild->WorldAlignMaxs(), MASK_NPCSOLID, pChild, COLLISION_GROUP_NONE, &tr);
		//NDebugOverlay::Box( pChild->GetAbsOrigin(), pChild->WorldAlignMins(), pChild->WorldAlignMaxs(), 0, 255, 0, 32, 5.0 );
		if (tr.fraction != 1.0 && tr.m_pEnt)
		{
			if (FClassnameIs(tr.m_pEnt, "prop_physics"))
			{
				// Set to non-solid so this loop doesn't keep finding it
				tr.m_pEnt->AddSolidFlags(FSOLID_NOT_SOLID);
				UTIL_RemoveImmediate(tr.m_pEnt);
				continue;
			}
		}

		bFound = false;
	}
}

void CLogicAssault::InputEnable(inputdata_t& inputdata)
{
	Enable();
}

void CLogicAssault::InputDisable(inputdata_t& inputdata)
{
	Disable();
}

