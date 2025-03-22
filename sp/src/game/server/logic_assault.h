#ifndef LOGICASSAULT_H
#define LOGICASSAULT_H
#include "cbase.h"
#include "monstermaker.h"

class CLogicAssault : public CLogicalEntity
{
	DECLARE_CLASS(CLogicAssault, CLogicalEntity);
public:
	CLogicAssault();
	void Spawn(void);
	void Precache(void);
	void AssaultThink(void);
	void SUB_DoNothing(void) {};
	bool HumanHullFits(const Vector& vecLocation, CBaseEntity* pIgnoreEntity = NULL);
	bool CanMakeNPC(bool bIgnoreSolidEntities = false, CNPCSpawnDestination* pSpawnPoint = NULL);
	CNPCSpawnDestination* FindSpawnDestination();
	void DeathNotice(CBaseEntity* pChild);
	void MakeNPC(void);
	void Enable(void);
	void Disable(void);
	void ChildPostSpawn(CAI_BaseNPC* pChild);
	const char* GetEnemyType();
	virtual bool KeyValue(const char* szKeyName, const char* szValue);

	DECLARE_DATADESC();
private:
	int m_iMaxEnemies;

	bool m_bDisabled;
	bool m_bEndlessWaves; // if enabled, the m_iMaxWaves value is ignored and waves go on and on
	int m_iNumEnemies; // num enemies in a wave to keep track of
	int m_iNumWave; // the wave number to keep track of, when spawning enemy squads
	int m_iMaxWaves; // the maximum number of waves the assault will have
	int m_iPhase; // the phase of this wave, each wave has three phases before proceeding to the next wave and resetting this value
	int m_iMinEnemiesToEndWave; // how many enemies there can be on the map before the wave ends
	string_t m_EnemyType;
	string_t m_EnemyModel;

	float m_fShotgunChance;
	float m_fAR2Chance;
	float m_fGrenadeChance;
	int m_iGrenadeCount;

	float m_flSpawnFrequency; // how often spawn attempts occur
	float m_flTimeUntilNextWave; // time until next wave

	float m_fEnemyCitizenMedicChance;

	int m_iTacticalVariant;

	string_t m_SpawnType;
	float m_fSpawnDistance;

	bool m_bShouldUpdateEnemies;
	bool m_bIsAssaultActivated;

	void InputEnable(inputdata_t& inputdata);
	void InputDisable(inputdata_t& inputdata);
	void InputChangeModel(inputdata_t& inputdata);

	COutputEHANDLE m_OnSpawnNPC; // Fired when the wave spawns an NPC (!activator is the NPC)
	COutputInt m_OnNextWave; // Fired when the next wave arrives
	COutputEvent m_OnAllWavesDefeated; // Fired when all waves are defeated (only if endless waves isn't set)
	COutputEvent m_OnAssaultStart; // Fired when the assault starts (happens once per entity)
};

#endif