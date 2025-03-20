#include "cbase.h"
#include "monstermaker.h"

class CLogicAssault : public CLogicalEntity
{
	DECLARE_CLASS(CLogicAssault, CLogicalEntity);

	void Spawn(void);
	void AssaultThink(void);
	void SUB_DoNothing(void) {};
	bool HumanHullFits(const Vector& vecLocation);
	bool CanMakeNPC(bool bIgnoreSolidEntities = false, CBaseEntity* m_hIgnoreEntity);
	CNPCSpawnDestination* FindSpawnDestination();
	virtual void DeathNotice(CBaseEntity* pChild);
	virtual void MakeNPC(void) = 0;
	virtual void Enable(void);
	virtual void Disable(void);
	virtual	void ChildPostSpawn(CAI_BaseNPC* pChild);
	virtual const char* GetEnemyType();

	DECLARE_DATADESC();

	int m_iMaxEnemies;
	int m_iMaxEnemiesPerWave;

	bool m_bDisabled;
	bool m_bEndlessWaves; // if enabled, the m_iMaxWaves value is ignored and waves go on and on
	int m_iNumEnemies; // max enemies in a wave
	int m_iNumWave; // the wave number to keep track of, when spawning enemy squads
	int m_iMaxWaves; // the maximum number of waves the assault will have
	int m_iPhase; // the phase of this wave, each wave has three phases before proceeding to the next wave and resetting this value
	int m_iMinEnemiesToEndWave; // how many enemies there can be on the map before the wave ends
	const char* m_EnemyType;
	const char* m_EnemyModel;

	float m_fShotgunChance;
	float m_fAR2Chance;
	float m_fGrenadeChance;
	int m_iGrenadeCount;

	float m_flSpawnFrequency;

	float m_fEnemyCitizenMedicChance;

	const char* m_TacticalVariant;

	const char* m_SpawnType;
	float m_fSpawnDistance;

	void InputEnable(inputdata_t& inputdata);
	void InputDisable(inputdata_t& inputdata);

	COutputEHANDLE m_OnSpawnNPC; // Fired when the wave spawns an NPC (!activator is the NPC)
	COutputEvent m_OnNextWave; // Fired when the next wave arrives
	COutputEvent m_OnAllWavesDefeated; // Fired when all waves are defeated (only if endless waves isn't set)
};