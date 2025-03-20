#include "cbase.h"
#include "monstermaker.h"

class CLogicAssault : public CLogicalEntity
{
	DECLARE_CLASS(CLogicAssault, CLogicalEntity);

	void Spawn(void);
	void AssaultThink(void);
	void SUB_DoNothing(void) {};
	bool HumanHullFits(const Vector& vecLocation, CBaseEntity* m_hIgnoreEntity);
	bool CanMakeNPC(bool bIgnoreSolidEntities = false, CBaseEntity* m_hIgnoreEntity);

	DECLARE_DATADESC();

	bool m_bDisabled;
	int m_iNumEnemies;

	COutputEHANDLE m_OnSpawnNPC; // Fired when the wave spawns an NPC (!activator is the NPC)
	COutputEvent m_OnFirstWave; // Fired on the first wave
	COutputEvent m_OnNextWave; // Fired when the next wave hits
	COutputEvent m_OnWaveDefeated; // Fired when the wave is defeated
	COutputEvent m_OnAllWavesDefeated; // Fired when all waves are defeated (only if endless waves isn't set)
};