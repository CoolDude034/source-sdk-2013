//=============================================================================//
//
// Purpose: A halloween pumpkin for Halloween event.
//
//=============================================================================//

#include "cbase.h"
#include "props.h"
#include "globalstate.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CPumpkin : public CPhysicsProp
{
	DECLARE_CLASS(CPumpkin, CPhysicsProp);
	DECLARE_DATADESC();
public:
	CPumpkin();

	virtual int OnTakeDamage(const CTakeDamageInfo& info);
	virtual void OnBreak(const Vector& vecVelocity, const AngularImpulse& angVel, CBaseEntity* pBreaker);
	void Spawn();
	void Think();
};

LINK_ENTITY_TO_CLASS(prop_pumpkin, CPumpkin);

BEGIN_DATADESC(CPumpkin)
DEFINE_THINKFUNC(Think)
END_DATADESC()

CPumpkin::CPumpkin()
{
	SetMaxHealth(8);
	SetHealth(8);
	AddSpawnFlags(8388608); // Zombies can't swat this

	string_t globalName = MAKE_STRING("pumpkins_destroyed");
	if (GlobalEntity_GetIndex(globalName) == -1)
	{
		GlobalEntity_Add(globalName, gpGlobals->mapname, GLOBAL_ON);
	}
}

void CPumpkin::Spawn(void)
{
	SetModelName(MAKE_STRING("models/props_outland/pumpkin01.mdl"));
	Precache();
	SetModel(STRING(GetModelName()));

	BaseClass::Spawn();

	SetThink(&CPumpkin::Think);
	SetNextThink(gpGlobals->curtime + 1.0f);
}

void CPumpkin::Think(void)
{
}

int CPumpkin::OnTakeDamage(const CTakeDamageInfo& info)
{
	int result = BaseClass::OnTakeDamage(info);
	return result;
}

void CPumpkin::OnBreak(const Vector& vecVelocity, const AngularImpulse& angVel, CBaseEntity* pBreaker)
{
	BaseClass::OnBreak(vecVelocity, angVel, pBreaker);

	string_t globalName = MAKE_STRING("pumpkins_destroyed");

	GlobalEntity_AddToCounter(globalName, 1);
}
