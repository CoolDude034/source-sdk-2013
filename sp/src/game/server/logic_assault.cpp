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