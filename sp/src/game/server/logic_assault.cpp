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

#define GENERIC_ASSAULT_FILE "scripts/assault_wavedata.txt"
#define LEVEL_ASSAULT_TWEAK_FILE UTIL_VarArgs("maps/%s_assault_wavedata.txt", STRING(gpGlobals->mapname))

#define ASSAULT_SPAWN_POINT_NAME "info_assault_spawn_point"

inline bool GetAssaultSettingsKeyValues(KeyValues* pKeyValues)
{
	// Try to load a map's specific wave data
	// If that fails, just load the global one
	if (pKeyValues->LoadFromFile(filesystem, LEVEL_ASSAULT_TWEAK_FILE, "MOD"))
		return true;
	return pKeyValues->LoadFromFile(filesystem, GENERIC_ASSAULT_FILE, "MOD");
}

LINK_ENTITY_TO_CLASS(logic_assault, CLogicAssault);

BEGIN_DATADESC(CLogicAssault)
//DEFINE_KEYFIELD(m_bEndlessWaves, FIELD_BOOLEAN, "EndlessWaves"),

DEFINE_OUTPUT(m_OnSpawnNPC, "OnSpawnNPC"),
DEFINE_OUTPUT(m_OnNextWave, "OnNextWave"),
DEFINE_OUTPUT(m_OnAllWavesDefeated, "OnAllWaveDefeated"),
END_DATADESC()

void CLogicAssault::Spawn(void)
{
	m_iNumWave = 1;
	m_iPhase = 0;
	KeyValues* assaultdata = new KeyValues("AssaultWaveData");
	if (GetAssaultSettingsKeyValues(assaultdata))
	{
		m_bEndlessWaves = assaultdata->GetBool("EndlessWaves", true);
		m_iMaxEnemies = assaultdata->GetInt("MaxEnemies", 8);
		m_iMaxEnemiesPerWave = assaultdata->GetInt("MaxEnemiesPerWave", 4);
		m_iMaxWaves = assaultdata->GetInt("MaxWaves", 5);
		m_iMinEnemiesToEndWave = assaultdata->GetInt("MinEnemiesToEndWave", 1);
		m_EnemyType = assaultdata->GetString("EnemyType", "combine");
		m_EnemyModel = assaultdata->GetString("EnemyModel", "models/combine_soldier.mdl");
		m_fShotgunChance = assaultdata->GetFloat("ShotgunChance", 0.25F);
		m_fAR2Chance = assaultdata->GetFloat("AR2Chance", 0.15F);
		m_fGrenadeChance = assaultdata->GetFloat("GrenadeChance", 0.25F);
		m_fEnemyCitizenMedicChance = assaultdata->GetFloat("MedicChance", 0.15F);
		m_TacticalVariant = assaultdata->GetString("TacticalVariant", "2");
		m_flSpawnFrequency = assaultdata->GetFloat("SpawnFrequency", 0.1F);
		m_SpawnType = assaultdata->GetString("SpawnType", "random");
		m_fSpawnDistance = assaultdata->GetFloat("SpawnDistance", 0.1F);

		assaultdata->deleteThis();
	}
	RunScriptFile("assault_wave_manager"); // Run the assault_wave_manager VScript
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
	SetNextThink(gpGlobals->curtime + m_flSpawnFrequency);
	MakeNPC();
}

//-----------------------------------------------------------------------------
// A not-very-robust check to see if a human hull could fit at this location.
// used to validate spawn destinations.
//-----------------------------------------------------------------------------
bool CLogicAssault::HumanHullFits(const Vector& vecLocation)
{
	trace_t tr;
	UTIL_TraceHull(vecLocation,
		vecLocation + Vector(0, 0, 1),
		NAI_Hull::Mins(HULL_HUMAN),
		NAI_Hull::Maxs(HULL_HUMAN),
		MASK_NPCSOLID,
		NULL,
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
	if (m_iNumEnemies >= m_iMaxEnemies)
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

					if (!HumanHullFits(tr.endpos + Vector(0, 0, 1)))
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

#define MAX_DESTINATION_ENTS	100
CNPCSpawnDestination* CLogicAssault::FindSpawnDestination()
{
	CNPCSpawnDestination* pDestinations[MAX_DESTINATION_ENTS];
	CBaseEntity* pEnt = NULL;
	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
	int	count = 0;

	if (!pPlayer)
	{
		return NULL;
	}

	// Collect all the qualifiying destination ents
	pEnt = gEntList.FindEntityByName(NULL, ASSAULT_SPAWN_POINT_NAME);

	if (!pEnt)
	{
		DevWarning("logic_assault doesn't have any spawn destinations!\n");
		return NULL;
	}

	while (pEnt)
	{
		CNPCSpawnDestination* pDestination;

		pDestination = dynamic_cast <CNPCSpawnDestination*>(pEnt);

		if (pDestination && pDestination->IsAvailable())
		{
			bool fValid = true;
			Vector vecTest = pDestination->GetAbsOrigin();

			// Right now View Cone check is omitted intentionally.
			Vector vecTopOfHull = NAI_Hull::Maxs(HULL_HUMAN);
			vecTopOfHull.x = 0;
			vecTopOfHull.y = 0;
			bool fVisible = (pPlayer->FVisible(vecTest) || pPlayer->FVisible(vecTest + vecTopOfHull));

			if (!fVisible)
				fValid = false;
			else
			{
				if (!(pPlayer->GetFlags() & FL_NOTARGET))
					fValid = false;
			}

			if (fValid)
			{
				pDestinations[count] = pDestination;
				count++;
			}
		}

		pEnt = gEntList.FindEntityByName(pEnt, ASSAULT_SPAWN_POINT_NAME);
	}

	if (count < 1)
		return NULL;

	// Now find the nearest/farthest based on distance criterion
	if (m_SpawnType == "random")
	{
		// Pretty lame way to pick randomly. Try a few times to find a random
		// location where a hull can fit. Don't try too many times due to performance
		// concerns.
		for (int i = 0; i < 5; i++)
		{
			CNPCSpawnDestination* pRandomDest = pDestinations[rand() % count];

			if (HumanHullFits(pRandomDest->GetAbsOrigin()))
			{
				return pRandomDest;
			}
		}

		return NULL;
	}
	else
	{
		if (m_SpawnType == "nearest")
		{
			float flNearest = FLT_MAX;
			CNPCSpawnDestination* pNearest = NULL;

			for (int i = 0; i < count; i++)
			{
				Vector vecTest = pDestinations[i]->GetAbsOrigin();
				float flDist = (vecTest - pPlayer->GetAbsOrigin()).Length();

				if (m_fSpawnDistance != 0 && m_fSpawnDistance > flDist)
					continue;

				if (flDist < flNearest && HumanHullFits(vecTest))
				{
					flNearest = flDist;
					pNearest = pDestinations[i];
				}
			}

			return pNearest;
		}
		else
		{
			float flFarthest = 0;
			CNPCSpawnDestination* pFarthest = NULL;

			for (int i = 0; i < count; i++)
			{
				Vector vecTest = pDestinations[i]->GetAbsOrigin();
				float flDist = (vecTest - pPlayer->GetAbsOrigin()).Length();

				if (m_fSpawnDistance != 0 && m_fSpawnDistance > flDist)
					continue;

				if (flDist > flFarthest && HumanHullFits(vecTest))
				{
					flFarthest = flDist;
					pFarthest = pDestinations[i];
				}
			}

			return pFarthest;
		}
	}

	return NULL;
}

void CLogicAssault::DeathNotice(CBaseEntity* pVictim)
{
	// ok, we've gotten the deathnotice from our child, now clear out its owner if we don't want it to fade.
	m_iNumEnemies--;

	// If we're here, we're getting erroneous death messages from children we haven't created
	AssertMsg(m_iNumEnemies >= 0, "logic_assault receiving child death notice but thinks has no children\n");

	// If all enemies in this wave are defeated or there is one member left, proceed to the next wave
	if (m_iNumEnemies <= m_iMinEnemiesToEndWave)
	{
		// If we beat all three phases, go to the next wave
		if (m_iPhase > 3)
		{
			m_iPhase = 0;
			m_iNumWave++;
			m_OnNextWave.FireOutput(this, this);

			if (m_iNumWave > m_iMaxWaves && !m_bEndlessWaves)
			{
				Disable();
				m_OnAllWavesDefeated.FireOutput(this, this);
			}
		}
		else
		{
			// Go to the next phase
			m_iPhase++;
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

	CNPCSpawnDestination* pSpawnPoint = FindSpawnDestination();
	if (pSpawnPoint == NULL)
		return;

	CAI_BaseNPC* pent = (CAI_BaseNPC*)CreateEntityByName(GetEnemyType());

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
		pent->SetModelName(AllocPooledString(m_EnemyModel));
		if (m_TacticalVariant == "0" || m_TacticalVariant == "1" || m_TacticalVariant == "2" || m_TacticalVariant == "3")
		{
			pent->KeyValue("tacticalvariant", m_TacticalVariant);
		}
		else
		{
			pent->KeyValue("tacticalvariant", "1");
		}
		if (random->RandomFloat() < m_fShotgunChance)
		{
			pent->m_spawnEquipment = AllocPooledString("weapon_shotgun");
		}
		else if (random->RandomFloat() < m_fAR2Chance)
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
		pent->SetModelName(AllocPooledString("models/elite_police.mdl"));
		if (random->RandomFloat() < m_fShotgunChance)
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
		pent->AddSpawnFlags(262144); // Random head
		pent->KeyValue("citizentype", "3"); // Rebel Citizen
		if (random->RandomFloat() < m_fShotgunChance)
		{
			pent->m_spawnEquipment = AllocPooledString("weapon_shotgun");
		}
		else
		{
			pent->m_spawnEquipment = AllocPooledString("weapon_smg1");
		}
	}
	if (random->RandomFloat() < m_fGrenadeChance)
	{
		pent->KeyValue("NumGrenades", "5");
	}
	if (random->RandomFloat() < m_fEnemyCitizenMedicChance)
	{
		pent->AddSpawnFlags(131072); // Medic
	}
	pent->AddSpawnFlags(1024); // Think outside of PVS
	pent->SetSquadName(AllocPooledString("squad_assault_group"));
	pent->SetHintGroup(AllocPooledString("hint_assault"));

	DispatchSpawn(pent);
	pent->SetOwnerEntity(this);
	ChildPostSpawn(pent);

	CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
	if (pPlayer)
	{
		pent->UpdateEnemyMemory(NULL, pPlayer->GetAbsOrigin());
	}

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

const char* CLogicAssault::GetEnemyType()
{
	if (m_EnemyType == "combine")
		return "npc_combine_s";
	if (m_EnemyType == "cops")
		return "npc_metropolice";
	if (m_EnemyType == "rebels")
		return "npc_citizen";
	return "npc_combine_s";
}

void CLogicAssault::InputEnable(inputdata_t& inputdata)
{
	Enable();
}

void CLogicAssault::InputDisable(inputdata_t& inputdata)
{
	Disable();
}

