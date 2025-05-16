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

int g_numPumpkinsDestroyed = 0;

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

	g_numPumpkinsDestroyed++;
	GlobalEntity_AddToCounter("pumpkins_destroyed", g_numPumpkinsDestroyed);

	DevMsg("Pumpkin destroyed! Total destroyed: %d\n", g_numPumpkinsDestroyed);
}
