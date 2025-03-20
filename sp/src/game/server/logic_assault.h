#include "cbase.h"
#include "monstermaker.h"

enum EnemyType {
	ENEMY_TYPE_COMBINE = 0,
	ENEMY_TYPE_REBEL,
	ENEMY_TYPE_COPS,
};

class CLogicAssault : public CLogicalEntity
{
	DECLARE_CLASS(CLogicAssault, CLogicalEntity);

	void Spawn(void);
	void AssaultThink(void);
	void SUB_DoNothing(void) {};
	bool HumanHullFits(const Vector& vecLocation, CBaseEntity* m_hIgnoreEntity);
	bool CanMakeNPC(bool bIgnoreSolidEntities = false, CBaseEntity* m_hIgnoreEntity);
	CNPCSpawnDestination* GetNearbySpawnPoint();
	virtual void DeathNotice(CBaseEntity* pChild);
	virtual void MakeNPC(void) = 0;
	virtual void Enable(void);
	virtual void Disable(void);
	virtual	void ChildPostSpawn(CAI_BaseNPC* pChild);

	DECLARE_DATADESC();

	int m_iMaxEnemies;
	int m_iMaxEnemiesPerWave;

	bool m_bDisabled;
	bool m_bEndlessWaves; // if enabled, the m_iMaxWaves value is ignored and waves go on and on
	bool m_bIsFirstWaveTriggered; // guard-check for first wave triggers
	int m_iNumEnemies; // max enemies in a wave
	int m_iNumWave; // the wave number to keep track of, when spawning enemy squads
	int m_iMaxWaves; // the maximum number of waves the assault will have
	int m_iPhase; // the phase of this wave, each wave has three phases before proceeding to the next wave and resetting this value
	EnemyType m_EnemyType;
	const char* m_EnemyModel;

	float m_fShotgunChance;
	float m_fAR2Chance;
	float m_fGrenadeChance;
	int m_iGrenadeCount;

	float m_flSpawnFrequency;

	float m_fEnemyCitizenMedicChance;

	const char* m_TacticalVariant;

	void InputEnable(inputdata_t& inputdata);
	void InputDisable(inputdata_t& inputdata);

	COutputEHANDLE m_OnSpawnNPC; // Fired when the wave spawns an NPC (!activator is the NPC)
	COutputEvent m_OnFirstPhase; // Fired on the first phase
	COutputEvent m_OnNextPhase; // Fired when the next phase hits
	COutputEvent m_OnWaveDefeated; // Fired when the wave is defeated
	COutputEvent m_OnAllWavesDefeated; // Fired when all waves are defeated (only if endless waves isn't set)
};