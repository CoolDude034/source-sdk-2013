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
#include "npc_citizen17.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GENERIC_ASSAULT_FILE "scripts/assault_wavedata.txt"
#define ASSAULT_SPAWN_POINT_NAME "info_assault_spawn_point"

inline bool GetAssaultSettingsKeyValues(KeyValues* pKeyValues)
{
	return pKeyValues->LoadFromFile(filesystem, GENERIC_ASSAULT_FILE, "MOD");
}

LINK_ENTITY_TO_CLASS(logic_assault, CLogicAssault);

BEGIN_DATADESC(CLogicAssault)

DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "Disabled"),

//DEFINE_FIELD(m_bEndlessWaves, FIELD_BOOLEAN),
DEFINE_FIELD(m_iMaxEnemies, FIELD_INTEGER),
DEFINE_FIELD(m_iMinEnemiesToKillToProgress, FIELD_INTEGER),
DEFINE_FIELD(m_iMinEnemiesToEndWave, FIELD_INTEGER),
//DEFINE_FIELD(m_EnemyType, FIELD_STRING),
//DEFINE_FIELD(m_EnemyModel, FIELD_MODELNAME),
DEFINE_KEYFIELD(m_bUseOverrides, FIELD_BOOLEAN, "UseOverrides"),
DEFINE_KEYFIELD(m_bEndlessWaves, FIELD_BOOLEAN, "EndlessWaves"),
DEFINE_KEYFIELD(m_EnemyType, FIELD_STRING, "EnemyTypeOverride"),
DEFINE_KEYFIELD(m_EnemyModel, FIELD_MODELNAME, "EnemyModelOverride"),
DEFINE_KEYFIELD(m_SpawnType, FIELD_STRING, "SpawnType"),
DEFINE_KEYFIELD(m_fSpawnDistance, FIELD_FLOAT, "SpawnDistance"),
DEFINE_KEYFIELD(m_bShouldUpdateEnemies, FIELD_BOOLEAN, "ShouldUpdateEnemies"),
DEFINE_KEYFIELD(m_iMaxWaves, FIELD_INTEGER, "MaxWaves"),

DEFINE_FIELD(m_fShotgunChance, FIELD_FLOAT),
DEFINE_FIELD(m_fAR2Chance, FIELD_FLOAT),
DEFINE_FIELD(m_fGrenadeChance, FIELD_FLOAT),
DEFINE_FIELD(m_fEnemyCitizenMedicChance, FIELD_FLOAT),
DEFINE_FIELD(m_iTacticalVariant, FIELD_INTEGER),
DEFINE_FIELD(m_flSpawnFrequency, FIELD_FLOAT),
DEFINE_FIELD(m_flTimeUntilNextWave, FIELD_TIME),
//DEFINE_FIELD(m_SpawnType, FIELD_STRING),
//DEFINE_FIELD(m_fSpawnDistance, FIELD_FLOAT),
//DEFINE_FIELD(m_bShouldUpdateEnemies, FIELD_BOOLEAN),

DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
DEFINE_INPUTFUNC(FIELD_MODELNAME, "ChangeModel", InputChangeModel),

DEFINE_OUTPUT(m_OnSpawnNPC, "OnSpawnNPC"),
DEFINE_OUTPUT(m_OnNextWave, "OnNextWave"),
DEFINE_OUTPUT(m_OnAllWavesDefeated, "OnAllWaveDefeated"),
DEFINE_OUTPUT(m_OnAssaultStart, "OnAssaultStart"),
END_DATADESC()

static string_t SPAWN_TYPE_RANDOM = AllocPooledString("random");
static string_t SPAWN_TYPE_NEAREST = AllocPooledString("nearest");

CLogicAssault::CLogicAssault()
{
	m_iNumWave = 1;
	m_iPhase = 0;
	KeyValues* assaultdata = new KeyValues("AssaultWaveData");
	if (GetAssaultSettingsKeyValues(assaultdata))
	{
		m_iMinEnemiesToKillToProgress = assaultdata->GetInt("MinEnemiesToKillToProgress", 8);
		m_iMinEnemiesToEndWave = assaultdata->GetInt("MinEnemiesToEndWave", 1);
		if (!m_bUseOverrides)
		{
			m_EnemyType = AllocPooledString(assaultdata->GetString("EnemyType", "combine"));
			m_EnemyModel = AllocPooledString(assaultdata->GetString("EnemyModel", "models/combine_soldier.mdl"));
			m_SpawnType = AllocPooledString(assaultdata->GetString("SpawnType", "random"));
			m_fSpawnDistance = assaultdata->GetFloat("SpawnDistance", 400.0F);
			m_bShouldUpdateEnemies = assaultdata->GetBool("ShouldUpdateEnemies", true);
			m_bEndlessWaves = assaultdata->GetBool("EndlessWaves", true);
			m_iMaxEnemies = assaultdata->GetInt("MaxEnemies", 8);
			m_iMaxWaves = assaultdata->GetInt("MaxWaves", 5);
		}
		m_fShotgunChance = assaultdata->GetFloat("ShotgunChance", 0.25F);
		m_fAR2Chance = assaultdata->GetFloat("AR2Chance", 0.15F);
		m_fGrenadeChance = assaultdata->GetFloat("GrenadeChance", 0.25F);
		m_fEnemyCitizenMedicChance = assaultdata->GetFloat("MedicChance", 0.15F);
		m_iTacticalVariant = assaultdata->GetInt("TacticalVariant", 2);
		m_flSpawnFrequency = assaultdata->GetFloat("SpawnFrequency", 0.1F);
		m_flTimeUntilNextWave = assaultdata->GetFloat("TimeUntilNextWave", 1.0F);

		assaultdata->deleteThis();
	}
	m_iMinEnemiesToKillToProgressCounter = m_iMinEnemiesToKillToProgress;

	m_iszVScripts = AllocPooledString("assault_globals");
}

void CLogicAssault::Spawn(void)
{
	if (m_bDisabled)
	{
		//wait to be activated.
		SetThink(&CLogicAssault::SUB_DoNothing);
	}
	else
	{
		SetThink(&CLogicAssault::AssaultThink);
		SetNextThink(gpGlobals->curtime + 0.1f);
	}
}

void CLogicAssault::Precache(void)
{
	PrecacheModel(m_EnemyModel.ToCStr());
}

void CLogicAssault::AssaultThink(void)
{
	if (m_bDisabled) return;
	if (m_iNumEnemies < m_iMaxEnemies && m_iNumWave <= m_iMaxWaves && !m_bEndlessWaves)
	{
		MakeNPC();
	}

	if (m_iMinEnemiesToKillToProgressCounter < m_iMinEnemiesToEndWave)
	{
		// If we beat all three phases, go to the next wave
		if (m_iPhase > 3)
		{
			m_iPhase = 0;
			m_iNumWave++;
			variant_t variant;
			variant.SetInt(m_iNumWave);
			m_OnNextWave.FireOutput(variant, this, this);

			// If all waves are complete and not endless, end the assault
			if (m_iNumWave > m_iMaxWaves && !m_bEndlessWaves)
			{
				m_OnAllWavesDefeated.FireOutput(this, this);
				Disable();
				return;
			}

			SetNextThink(gpGlobals->curtime + m_flTimeUntilNextWave);
			return;
		}
		else
		{
			// Go to the next phase
			m_iPhase++;
		}
	}

	SetNextThink(gpGlobals->curtime + m_flSpawnFrequency);
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
bool CLogicAssault::CanMakeNPC(bool bIgnoreSolidEntities, CNPCSpawnDestination* pSpawnPoint)
{
	if (m_iNumEnemies > m_iMaxEnemies)
	{
		return false;
	}

	Vector mins = pSpawnPoint->GetAbsOrigin() - Vector(34, 34, 0);
	Vector maxs = pSpawnPoint->GetAbsOrigin() + Vector(34, 34, 0);
	maxs.z = pSpawnPoint->GetAbsOrigin().z;

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
			if (pPlayer->FInViewCone(pSpawnPoint->GetAbsOrigin()) && pPlayer->FVisible(pSpawnPoint->GetAbsOrigin()))
			{
				if ((pPlayer->GetFlags() & FL_NOTARGET))
					return true;
				return false;
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
	if (m_SpawnType == SPAWN_TYPE_RANDOM)
	{
		// Pretty lame way to pick randomly. Try a few times to find a random
		// location where a hull can fit. Don't try too many times due to performance
		// concerns.
		for (int i = 0; i < 5; i++)
		{
			CNPCSpawnDestination* pRandomDest = pDestinations[rand() % count];

			if (HumanHullFits(pRandomDest->GetAbsOrigin(), pRandomDest))
			{
				return pRandomDest;
			}
		}

		return NULL;
	}
	else
	{
		if (m_SpawnType == SPAWN_TYPE_NEAREST)
		{
			float flNearest = FLT_MAX;
			CNPCSpawnDestination* pNearest = NULL;

			for (int i = 0; i < count; i++)
			{
				Vector vecTest = pDestinations[i]->GetAbsOrigin();
				float flDist = (vecTest - pPlayer->GetAbsOrigin()).Length();

				if (m_fSpawnDistance != 0 && m_fSpawnDistance > flDist)
					continue;

				if (flDist < flNearest && HumanHullFits(vecTest, pNearest))
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

				if (flDist > flFarthest && HumanHullFits(vecTest, pFarthest))
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

	if (m_iMinEnemiesToKillToProgressCounter > 0)
	{
		m_iMinEnemiesToKillToProgressCounter--;
	}
	else if (m_iMinEnemiesToKillToProgressCounter <= 0 && m_bEndlessWaves)
	{
		m_iMinEnemiesToKillToProgressCounter = m_iMinEnemiesToKillToProgress;
	}

	// If we're here, we're getting erroneous death messages from children we haven't created
	AssertMsg(m_iNumEnemies >= 0, "logic_assault receiving child death notice but thinks has no children\n");
}

//-----------------------------------------------------------------------------
// Purpose: Creates the NPC.
//-----------------------------------------------------------------------------
void CLogicAssault::MakeNPC(void)
{
	CNPCSpawnDestination* pSpawnPoint = FindSpawnDestination();
	if (pSpawnPoint == NULL)
		return;

	if (!CanMakeNPC(false, pSpawnPoint))
		return;

	CAI_BaseNPC* pent = (CAI_BaseNPC*)CreateEntityByName(GetEnemyType());

	if (!pent)
	{
		Warning("logic_assault failed to create NPC!\n");
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
		pent->SetModelName(m_EnemyModel);
		switch (m_iTacticalVariant)
		{
		case 0:
			pent->KeyValue("tacticalvariant", "0");
			break;
		case 1:
			pent->KeyValue("tacticalvariant", "1");
			break;
		case 2:
			pent->KeyValue("tacticalvariant", "2");
			break;
		case 3:
			pent->KeyValue("tacticalvariant", "3");
			break;
		default:
			pent->KeyValue("tacticalvariant", "1");
			break;
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
		pent->KeyValue("citizentype", "6"); // Rebel Citizen (hostile)
		if (random->RandomFloat() < m_fEnemyCitizenMedicChance)
		{
			pent->AddSpawnFlags(131072); // Chance for Enemy Medic
		}
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
	pent->AddSpawnFlags(1024); // Think outside of PVS
	pent->SetSquadName(AllocPooledString("squad_assault_group"));
	pent->SetHintGroup(AllocPooledString("hint_assault"));

	DispatchSpawn(pent);
	pent->SetOwnerEntity(this);
	ChildPostSpawn(pent);

	if (m_bShouldUpdateEnemies)
	{
		CBasePlayer* pPlayer = UTIL_GetLocalPlayer();
		if (pPlayer)
		{
			pent->UpdateEnemyMemory(NULL, pPlayer->GetAbsOrigin());
		}
	}

	m_iNumEnemies++;// count this NPC
}

void CLogicAssault::Enable(void)
{
	m_bDisabled = false;
	if (!m_bIsAssaultActivated)
	{
		m_bIsAssaultActivated = true;
		m_OnAssaultStart.FireOutput(this, this);
	}
	if (m_iNumWave <= 0)
	{
		m_iNumWave = 1;
	}
	SetThink(&CLogicAssault::AssaultThink);
	SetNextThink(gpGlobals->curtime);
}

void CLogicAssault::Disable(void)
{
	m_bDisabled = true;
	m_iNumWave = 1;
	m_iPhase = 0;
	m_iMinEnemiesToKillToProgressCounter = m_iMinEnemiesToKillToProgress;
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

static string_t CombineGroup = AllocPooledString("combine");
static string_t CombineGroup2 = AllocPooledString("cops");
static string_t RebelGroup = AllocPooledString("rebels");

const char* CLogicAssault::GetEnemyType()
{
	if (m_EnemyType == CombineGroup)
		return "npc_combine_s";
	if (m_EnemyType == CombineGroup2)
		return "npc_metropolice";
	if (m_EnemyType == RebelGroup)
		return "npc_citizen";
	return "npc_combine_s";
}

bool CLogicAssault::KeyValue(const char* szKeyName, const char* szValue)
{
	return BaseClass::KeyValue(szKeyName, szValue);
}

void CLogicAssault::InputEnable(inputdata_t& inputdata)
{
	Enable();
}

void CLogicAssault::InputDisable(inputdata_t& inputdata)
{
	Disable();
}

void CLogicAssault::InputChangeModel(inputdata_t& inputdata)
{
	m_EnemyType = AllocPooledString(inputdata.value.String());
}