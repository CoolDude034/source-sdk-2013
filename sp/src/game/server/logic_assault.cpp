//=============================================================================//
//
// Purpose: Central assault manager, spawns enemies trough assault waves similar to games like PAYDAY
//			Most of this code is from monstermaker.cpp but repurposed
//
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "monstermaker.h"
#include "mapentities.h"
#include "logic_assault.h"
#include "npc_citizen17.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sv_enable_weighted_system("sv_enable_weighted_system", "1", FCVAR_CHEAT | FCVAR_SPONLY);

LINK_ENTITY_TO_CLASS(logic_assault, CLogicAssault);

BEGIN_DATADESC(CLogicAssault)

DEFINE_KEYFIELD(m_bStartDisabled, FIELD_BOOLEAN, "StartDisabled"),
DEFINE_KEYFIELD(m_flInitialDelay, FIELD_FLOAT, "InitialDelay"), // initial delay set by hammer I/O

DEFINE_FIELD(m_iMaxEnemies, FIELD_INTEGER),
DEFINE_FIELD(m_iNumEnemies, FIELD_INTEGER),
DEFINE_FIELD(m_iNumAssaultWave, FIELD_INTEGER),
DEFINE_FIELD(g_assaultStage, FIELD_INTEGER),

DEFINE_FIELD(m_fShotgunChance, FIELD_FLOAT),
DEFINE_FIELD(m_fAR2Chance, FIELD_FLOAT),
DEFINE_FIELD(m_fGrenadeChance, FIELD_FLOAT),
DEFINE_FIELD(m_fEnemyMedicChance, FIELD_FLOAT),
DEFINE_FIELD(m_flSpawnFrequency, FIELD_FLOAT),
DEFINE_FIELD(m_flPhaseStartTime, FIELD_TIME),
DEFINE_FIELD(m_fDiff, FIELD_FLOAT),

DEFINE_FIELD(m_flBuildDuration, FIELD_FLOAT),
DEFINE_FIELD(m_flAssaultDuration, FIELD_FLOAT),
DEFINE_FIELD(m_flFadeDuration, FIELD_FLOAT),
DEFINE_FIELD(m_flAnticipationDuration, FIELD_FLOAT),

DEFINE_INPUTFUNC(FIELD_VOID, "StartAssault", InputStartAssault),

DEFINE_OUTPUT(m_OnAssaultStart, "OnAssaultStart"),
DEFINE_OUTPUT(m_OnAssaultEnd, "OnAssaultEnd"),
END_DATADESC()

void CLogicAssault::InputStartAssault(inputdata_t& inputdata)
{
	StartDirector();
}

CLogicAssault::CLogicAssault()
{
	KeyValues* pKeyValues = new KeyValues("AssaultWaveData");
	char fileName[MAX_PATH];
	// Big thanks to grizzledev on Source Engine discord
	// also this one, i had to get gbt for help :<
	sprintf_s(fileName, MAX_PATH, "maps/%s_assault_wavedata.txt", gpGlobals->mapname.ToCStr());
	if (pKeyValues->LoadFromFile(filesystem, fileName, "MOD"))
	{
		KeyValues* pSpawnPool = pKeyValues->FindKey("SpawnPool", true);
		if (pSpawnPool)
		{
			for (KeyValues* kvSubKey = pSpawnPool->GetFirstSubKey(); kvSubKey != NULL; kvSubKey = kvSubKey->GetNextKey())
			{
				SpawnEntry entry;
				entry.m_EnemyModel = MAKE_STRING(kvSubKey->GetString("EnemyModel", "models/combine_soldier.mdl"));
				entry.m_EnemyType = MAKE_STRING(kvSubKey->GetString("EnemyType", "combine"));
				entry.m_SpawnType = MAKE_STRING(kvSubKey->GetString("SpawnType", "normal")); // can be either, normal, elite, charger, suppressor, shield
				entry.m_WeaponOverride = MAKE_STRING(kvSubKey->GetString("WeaponOverride"));
				entry.m_SquadOverride = MAKE_STRING(kvSubKey->GetString("SquadOverride"));
				entry.m_HintOverride = MAKE_STRING(kvSubKey->GetString("HintOverride"));
				entry.m_weight = kvSubKey->GetFloat("Weight");
				m_SpawnPool.AddToTail(entry);
			}
		}
		KeyValues* pAssaultData = pKeyValues->FindKey("AssaultData", true);
		if (pAssaultData)
		{
			for (KeyValues* kvSubKey = pAssaultData->GetFirstSubKey(); kvSubKey != NULL; kvSubKey = kvSubKey->GetNextKey())
			{
				m_iMaxEnemies = kvSubKey->GetInt("MaxEnemies", 8);
				m_fShotgunChance = kvSubKey->GetFloat("ShotgunChance", 0.25F);
				m_fAR2Chance = kvSubKey->GetFloat("AR2Chance", 0.15F);
				m_fGrenadeChance = kvSubKey->GetFloat("GrenadeChance", 0.25F);
				m_fEnemyMedicChance = kvSubKey->GetFloat("EnemyMedicChance", 0.15F); // as of now, only rebels have medics
				m_fEnemyShieldChance = kvSubKey->GetFloat("EnemyShieldChance", 0.15F);
				m_flSpawnFrequency = kvSubKey->GetFloat("SpawnFrequency", 0.1F);

				// Retrieve this shit
				m_flBuildDuration = kvSubKey->GetFloat("BuildDuration", 15.0F);
				m_flAnticipationDuration = kvSubKey->GetFloat("AnticipationDuration", 5.0F);
				m_flAssaultDuration = kvSubKey->GetFloat("AssaultDuration", 150.0F);
				m_flFadeDuration = kvSubKey->GetFloat("FadeDuration", 15.0F);
			}
		}
		KeyValues* pSpawnData = pKeyValues->FindKey("SpawnData", true);
		if (pSpawnData)
		{
			// We iterate trough a list of positions and store them in an array
			for (KeyValues* kvSubKey = pSpawnData->GetFirstSubKey(); kvSubKey != NULL; kvSubKey = kvSubKey->GetNextKey())
			{
				SpawnPoint entry;
				Vector pVector;
				QAngle pAngle;
				UTIL_StringToVector(pVector.Base(), kvSubKey->GetString("Position"));
				UTIL_StringToVector((float*)&pAngle, kvSubKey->GetString("Rotation"));
				entry.pos = pVector;
				entry.rot = pAngle;
				entry.m_bShouldRappel = kvSubKey->GetBool("ShouldRappel");
				m_spawnPoints.AddToTail(entry);
			}
		}
		pKeyValues->deleteThis();
	}
}

void CLogicAssault::Spawn(void)
{
	g_assaultStage = ASSAULT_DIRECTOR_PHASE_CONTROL;
	if (m_bStartDisabled)
	{
		SetThink(&CLogicAssault::SUB_DoNothing);
	}
	else
	{
		SetThink(&CLogicAssault::AssaultThink);
		SetNextThink(gpGlobals->curtime); // think now!
		m_flPhaseStartTime = gpGlobals->curtime;
	}
}

void CLogicAssault::Precache(void)
{
	for (int i = 0; i < m_SpawnPool.Count(); i++)
	{
		const SpawnEntry& entry = m_SpawnPool[i];
		if (entry.m_EnemyModel != NULL_STRING)
		{
			PrecacheModel(entry.m_EnemyModel.ToCStr());
		}
	}
}

void CLogicAssault::AssaultThink(void)
{
	switch (g_assaultStage)
	{
	case ASSAULT_DIRECTOR_PHASE_CONTROL:
		if (gpGlobals->curtime >= m_flPhaseStartTime + m_flInitialDelay)
		{
			// We're in control phase, usually only happens once and is the first transition point when triggered by map IO
			g_assaultStage = ASSAULT_DIRECTOR_PHASE_BUILDUP;
			m_flPhaseStartTime = gpGlobals->curtime;
		}
		break;
	case ASSAULT_DIRECTOR_PHASE_BUILDUP:
		if (gpGlobals->curtime >= m_flPhaseStartTime + m_flBuildDuration)
		{
			// The assault is starting to build up...
			g_assaultStage = ASSAULT_DIRECTOR_PHASE_ANTICIPATION;
			m_flPhaseStartTime = gpGlobals->curtime;
		}
		break;
	case ASSAULT_DIRECTOR_PHASE_ANTICIPATION:
		if (gpGlobals->curtime >= m_flPhaseStartTime + m_flAnticipationDuration)
		{
			g_assaultStage = ASSAULT_DIRECTOR_PHASE_ASSAULT;
			m_flPhaseStartTime = gpGlobals->curtime;
		}
		break;
	case ASSAULT_DIRECTOR_PHASE_ASSAULT:
		if (m_fDiff > 0.0F && m_iNumEnemies < m_iMaxEnemies)
		{
			MakeNPC();
		}
		if (gpGlobals->curtime >= m_flPhaseStartTime + m_flAssaultDuration)
		{
			g_assaultStage = ASSAULT_DIRECTOR_PHASE_FADE;
			m_flPhaseStartTime = gpGlobals->curtime;
		}
		break;
	case ASSAULT_DIRECTOR_PHASE_FADE:
		if (gpGlobals->curtime >= m_flPhaseStartTime + m_flFadeDuration)
		{
			// Restart the assault phase
			g_assaultStage = ASSAULT_DIRECTOR_PHASE_BUILDUP;
			m_flPhaseStartTime = gpGlobals->curtime;
			m_iNumAssaultWave++;
		}
		break;
	default:
		break;
	}

	if (g_assaultStage == ASSAULT_DIRECTOR_PHASE_ASSAULT)
	{
		SetNextThink(gpGlobals->curtime + m_flSpawnFrequency);
	}
	else
	{
		SetNextThink(gpGlobals->curtime); // think now
	}
}

//-----------------------------------------------------------------------------
// A not-very-robust check to see if a human hull could fit at this location.
// used to validate spawn destinations.
//-----------------------------------------------------------------------------
bool CLogicAssault::HumanHullFits(const Vector& vecLocation, CBaseEntity* pIgnoreEntity)
{
	trace_t tr;
	UTIL_TraceHull(vecLocation,
		vecLocation + Vector(0, 0, 1),
		NAI_Hull::Mins(HULL_HUMAN),
		NAI_Hull::Maxs(HULL_HUMAN),
		MASK_NPCSOLID,
		pIgnoreEntity,
		COLLISION_GROUP_NONE,
		&tr);

	if (tr.fraction == 1.0)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not it is OK to make an NPC at this instant.
//-----------------------------------------------------------------------------
bool CLogicAssault::CanMakeNPC(bool bIgnoreSolidEntities, const Vector& pSpawnPoint)
{
	if (m_iNumEnemies > m_iMaxEnemies) return false;

	Vector mins = pSpawnPoint - Vector(34, 34, 0);
	Vector maxs = pSpawnPoint + Vector(34, 34, 0);
	maxs.z = pSpawnPoint.z;

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
						NULL,
						COLLISION_GROUP_NONE,
						&tr);

					if (!HumanHullFits(tr.endpos + Vector(0, 0, 1), pList[i]))
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
			if (pPlayer->FInViewCone(pSpawnPoint) && pPlayer->FVisible(pSpawnPoint))
			{
				if ((pPlayer->GetFlags() & FL_NOTARGET))
					return true;
				return false;
			}
		}
	}

	return true;
}

void CLogicAssault::DeathNotice(CBaseEntity* pVictim)
{
	// ok, we've gotten the deathnotice from our child, now clear out its owner if we don't want it to fade.
	m_iNumEnemies--;

	// If we're here, we're getting erroneous death messages from children we haven't created
	AssertMsg(m_iNumEnemies >= 0, "logic_assault receiving child death notice but thinks has no children\n");
}

//-----------------------------------------------------------------------------
// Purpose: Creates the NPC.
//-----------------------------------------------------------------------------
void CLogicAssault::MakeNPC(void)
{
	if (m_spawnPoints.Count() > 0 && m_SpawnPool.Count() > 0)
	{
		const SpawnEntry& entry = WeightedRandomSpawnEntry(); // m_SpawnPool[random->RandomInt(0, m_SpawnPool.Count() - 1)]
		const SpawnPoint& spawnData = m_spawnPoints[RandomInt(0, m_spawnPoints.Count() - 1)];

		if (CanMakeNPC(false, spawnData.pos))
		{
			CAI_BaseNPC* pent = (CAI_BaseNPC*)CreateEntityByName(GetEnemyType());

			if (!pent)
			{
				Warning("logic_assault failed to create NPC!\n");
				return;
			}

			pent->SetAbsOrigin(spawnData.pos);
			pent->SetAbsAngles(spawnData.rot);

			// Randomize direction the NPC is facing
			//QAngle angles;
			//angles.Init(0.0F, random->RandomFloat(), 0.0F);
			//pent->SetAbsAngles(angles);

			pent->AddSpawnFlags(SF_NPC_FALL_TO_GROUND);
			pent->AddSpawnFlags(SF_NPC_FADE_CORPSE);

			if (pent->ClassMatches("npc_combine_s"))
			{
				// can be either, normal, elite, charger, suppressor, shield
				if (entry.m_SpawnType == MAKE_STRING("elite"))
				{
					pent->SetModelName(MAKE_STRING("models/combine_super_soldier.mdl"));
					pent->KeyValue("additionalequipment", "weapon_ar2");
					pent->KeyValue("tacticalvariant", "1");
					pent->KeyValue("NumGrenades", "10");

					if (entry.m_WeaponOverride != NULL_STRING)
					{
						pent->m_spawnEquipment = entry.m_WeaponOverride;
					}
				}
				else if (entry.m_SpawnType == MAKE_STRING("charger"))
				{
					pent->KeyValue("IsCharger", "1");
				}
				else if (entry.m_SpawnType == MAKE_STRING("suppressor"))
				{
					pent->KeyValue("IsSuppressor", "1");
				}
				else if (entry.m_SpawnType == MAKE_STRING("shield"))
				{
					if (entry.m_EnemyModel != NULL_STRING)
					{
						pent->SetModelName(entry.m_EnemyModel);
					}
					pent->KeyValue("IsShield", "1");
				}
				else
				{
					if (entry.m_EnemyModel != NULL_STRING)
					{
						pent->SetModelName(entry.m_EnemyModel);
					}
					pent->KeyValue("tacticalvariant", "2"); // Pressure the Enemy until 25ft
					if (entry.m_WeaponOverride != NULL_STRING)
					{
						pent->m_spawnEquipment = entry.m_WeaponOverride;
					}
					else
					{
						if (random->RandomFloat() < m_fShotgunChance)
						{
							pent->m_spawnEquipment = MAKE_STRING("weapon_shotgun");
						}
						else if (random->RandomFloat() < m_fAR2Chance)
						{
							pent->m_spawnEquipment = MAKE_STRING("weapon_ar2");
						}
						else if (random->RandomFloat() < m_fEnemyShieldChance)
						{
							pent->KeyValue("IsShield", "1");
						}
						else
						{
							pent->m_spawnEquipment = MAKE_STRING("weapon_smg1");
						}
					}
					if (random->RandomFloat() < m_fGrenadeChance)
					{
						pent->KeyValue("NumGrenades", "5");
					}
				}
			}
			else if (pent->ClassMatches("npc_metropolice"))
			{
				if (entry.m_SpawnType == MAKE_STRING("simplecops"))
				{
					if (entry.m_EnemyModel != NULL_STRING)
					{
						pent->SetModelName(entry.m_EnemyModel);
					}
					pent->AddSpawnFlags(131072); // Simple Cops
					if (entry.m_WeaponOverride != NULL_STRING)
					{
						pent->m_spawnEquipment = entry.m_WeaponOverride;
					}
					else
					{
						if (random->RandomFloat() < m_fShotgunChance)
						{
							pent->m_spawnEquipment = MAKE_STRING("weapon_shotgun");
						}
						else
						{
							pent->m_spawnEquipment = MAKE_STRING("weapon_pistol");
						}
					}
				}
				else
				{
					pent->KeyValue("IsElite", "1");
					if (entry.m_WeaponOverride != NULL_STRING)
					{
						pent->m_spawnEquipment = entry.m_WeaponOverride;
					}
					else
					{
						if (random->RandomFloat() < m_fShotgunChance)
						{
							pent->m_spawnEquipment = MAKE_STRING("weapon_shotgun");
						}
						else
						{
							pent->m_spawnEquipment = MAKE_STRING("weapon_smg1");
						}
					}
					if (random->RandomFloat() < m_fGrenadeChance)
					{
						pent->KeyValue("NumGrenades", "5");
					}
				}
			}
			else if (pent->ClassMatches("npc_citizen"))
			{
				if (entry.m_EnemyModel != NULL_STRING)
				{
					pent->SetModelName(entry.m_EnemyModel);
					pent->KeyValue("citizentype", "4"); // Unique Citizen
					pent->KeyValue("hostile", "1");
				}
				else
				{
					pent->KeyValue("citizentype", "6"); // Rebel Citizen (hostile)
					pent->AddSpawnFlags(262144); // Random head
					if (random->RandomFloat() < m_fEnemyMedicChance)
					{
						pent->AddSpawnFlags(131072); // Chance for Enemy Medic
					}
					if (random->RandomFloat() < m_fGrenadeChance)
					{
						pent->KeyValue("NumGrenades", "5");
					}
				}
				if (entry.m_WeaponOverride != NULL_STRING)
				{
					pent->m_spawnEquipment = entry.m_WeaponOverride;
				}
				else
				{
					if (random->RandomFloat() < m_fShotgunChance)
					{
						pent->m_spawnEquipment = MAKE_STRING("weapon_shotgun");
					}
					else
					{
						pent->m_spawnEquipment = MAKE_STRING("weapon_smg1");
					}
				}
			}
			if (spawnData.m_bShouldRappel)
			{
				pent->KeyValue("waitingtorappel", "1");
			}
			pent->AddSpawnFlags(1024); // Think outside of PVS
			if (entry.m_SquadOverride != NULL_STRING)
			{
				pent->SetSquadName(entry.m_SquadOverride);
			}
			else
			{
				if (g_assaultStage == ASSAULT_DIRECTOR_PHASE_ASSAULT || g_assaultStage == ASSAULT_DIRECTOR_PHASE_BUILDUP || g_assaultStage == ASSAULT_DIRECTOR_PHASE_ANTICIPATION)
				{
					pent->SetSquadName(MAKE_STRING("squad_assault_group"));
				}
				else
				{
					pent->SetSquadName(MAKE_STRING("squad_recon_group"));
				}
			}
			if (entry.m_HintOverride != NULL_STRING)
			{
				pent->SetHintGroup(entry.m_HintOverride);
			}
			else
			{
				pent->SetHintGroup(MAKE_STRING("hint_assault"));
			}

			DispatchSpawn(pent);
			pent->SetOwnerEntity(this);
			ChildPostSpawn(pent);

			if (spawnData.m_bShouldRappel)
			{
				pent->BeginRappel();
			}

			CBasePlayer* pPlayer = UTIL_GetLocalPlayerOrListenServerHost();
			if (pPlayer)
			{
				pent->UpdateEnemyMemory(NULL, pPlayer->GetAbsOrigin());
			}

			m_iNumEnemies++;// count this NPC
		}
	}
}

void CLogicAssault::StartDirector(void)
{
	if (!m_bStartDisabled) return;
	SetThink(&CLogicAssault::AssaultThink);
	SetNextThink(gpGlobals->curtime);
	m_flPhaseStartTime = gpGlobals->curtime;
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
	for (int i = 0; i < m_SpawnPool.Count(); i++)
	{
		const SpawnEntry& entry = m_SpawnPool[i];
		if (entry.m_EnemyType != NULL_STRING)
		{
			if (entry.m_EnemyType == MAKE_STRING("combine"))
				return "npc_combine_s";
			if (entry.m_EnemyType == MAKE_STRING("police"))
				return "npc_metropolice";
			if (entry.m_EnemyType == MAKE_STRING("rebels"))
				return "npc_citizen";
		}
	}
	return "npc_combine_s";
}

bool CLogicAssault::KeyValue(const char* szKeyName, const char* szValue)
{
	return BaseClass::KeyValue(szKeyName, szValue);
}

const SpawnEntry& CLogicAssault::WeightedRandomSpawnEntry()
{
	if (sv_enable_weighted_system.GetBool())
	{
		float totalWeight = 0.0F;
		for (int i = 0; i < m_SpawnPool.Count(); i++)
		{
			totalWeight += m_SpawnPool[i].m_weight;
		}
		float randomWeight = random->RandomFloat(0, totalWeight);
		float current_weight = 0.0f;

		// pick best spawn entry
		for (int i = 0; i < m_SpawnPool.Count(); i++)
		{
			current_weight += m_SpawnPool[i].m_weight;
			if (randomWeight <= current_weight)
			{
				return m_SpawnPool[i];
			}
		}

		// pick random spawn entry
		return m_SpawnPool[random->RandomInt(0, m_SpawnPool.Count() - 1)];
	}

	// the system is disabled, so we just pick random one :[
	return m_SpawnPool[random->RandomInt(0, m_SpawnPool.Count() - 1)];
}
