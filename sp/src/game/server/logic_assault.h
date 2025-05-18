#ifndef LOGICASSAULT_H
#define LOGICASSAULT_H
#include "cbase.h"
#include "monstermaker.h"

enum AssaultStage
{
	ASSAULT_DIRECTOR_PHASE_CONTROL = 0,
	ASSAULT_DIRECTOR_PHASE_BUILDUP,
	ASSAULT_DIRECTOR_PHASE_ANTICIPATION,
	ASSAULT_DIRECTOR_PHASE_ASSAULT,
	ASSAULT_DIRECTOR_PHASE_FADE
};

struct SpawnEntry
{
	string_t m_EnemyType; // can either be "combine", "police" or "rebels"
	string_t m_EnemyModel;
	string_t m_SpawnType;
	string_t m_WeaponOverride;
	string_t m_SquadOverride;
	string_t m_HintOverride;
	float m_weight;
};

struct SpawnPoint
{
	Vector pos;
	QAngle rot;
	string_t m_hintGroup;
	bool m_bShouldRappel;
};

class CLogicAssault : public CLogicalEntity
{
	DECLARE_CLASS(CLogicAssault, CLogicalEntity);
	DECLARE_DATADESC();
public:
	CLogicAssault();
	void Spawn(void);
	void Precache(void);
	void AssaultThink(void);
	bool HumanHullFits(const Vector& vecLocation, CBaseEntity* pIgnoreEntity);
	bool CanMakeNPC(bool bIgnoreSolidEntities = false, const Vector& pSpawnPoint = vec3_origin);
	void SUB_DoNothing() {};
	void MakeNPC(void);
	void StartDirector(void);
	void ChildPostSpawn(CAI_BaseNPC* pChild);
	const char* GetEnemyType();
	virtual bool KeyValue(const char* szKeyName, const char* szValue);
private:
	bool m_bStartDisabled; // should the assault director start disabled
	int m_iMaxEnemies; // max number of enemies there can be at once (spawn cap)
	int m_iNumEnemies; // total enemies, counts each NPC spawned and killed
	int m_iNumAssaultWave; // assault wave counter
	int g_assaultStage;

	float m_fDiff; // assault intensity, yk like how payday does it

	float m_fShotgunChance;
	float m_fAR2Chance;
	float m_fGrenadeChance;
	int m_iGrenadeCount;
	float m_fEnemyMedicChance;
	float m_fEnemyShieldChance;

	float m_fNextSpawnTime;

	float m_flPhaseStartTime;
	float m_flAssaultDuration;
	float m_flFadeDuration;
	float m_flAnticipationDuration;
	float m_flBuildDuration;
	float m_flControlDuration;

	string_t m_SpawnType;
	float m_fSpawnDistance;
	float m_flSpawnFrequency; // how often spawn attempts occur

	const SpawnEntry& WeightedRandomSpawnEntry();

	void InputStartAssault(inputdata_t& inputdata);
	void InputSetDifficulty(inputdata_t& inputdata);

	void UpdateEnemies();

	CUtlVector<SpawnPoint> m_spawnPoints;
	CUtlVector<SpawnEntry> m_SpawnPool;
	CUtlVector<EHANDLE> m_spawnedEnemies;

	COutputEvent m_OnAssaultEnd; // Fired when the assault ends
	COutputEvent m_OnAssaultStart; // Fired when the assault starts
};

#endif