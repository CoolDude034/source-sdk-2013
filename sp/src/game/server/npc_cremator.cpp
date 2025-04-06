//=============================================================================//
//
// Purpose: Cremator, The Combine's janitors of City 17
//			keeping the city clean of dead bodies.
//
//			This NPC doesn't do much on it's own and it's intended for scripted sequences. (for now)
//
//=============================================================================//

#include	"cbase.h"
#include	"npcevent.h"
#include	"ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CREMATOR_MODEL_NAME "models/cremator.mdl"

class CNPC_Cremator : public CAI_BaseNPC
{
	DECLARE_CLASS(CNPC_Cremator, CAI_BaseNPC);
public:
	CNPC_Cremator();

	void	Spawn(void);
	void	Precache(void);
	Class_T Classify(void);
	int		GetSoundInterests(void);
};

LINK_ENTITY_TO_CLASS(npc_cremator, CNPC_Cremator);

CNPC_Cremator::CNPC_Cremator()
{
	m_iszVScripts = AllocPooledString("npcs/entities/cremator");
}

void CNPC_Cremator::Spawn(void)
{
	SetModelName(AllocPooledString(CREMATOR_MODEL_NAME));
	Precache();
	SetModel(CREMATOR_MODEL_NAME);
	SetHullType(HULL_MEDIUM);
	SetHullSizeNormal();
	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetMoveType(MOVETYPE_STEP);
	SetBloodColor(BLOOD_COLOR_YELLOW);
	m_iHealth = 80;
	m_flFieldOfView = 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState = NPC_STATE_NONE;

	CapabilitiesAdd(bits_CAP_MOVE_GROUND);

	AddEFlags(EFL_NO_DISSOLVE | EFL_NO_MEGAPHYSCANNON_RAGDOLL | EFL_NO_PHYSCANNON_INTERACTION);

	BaseClass::Spawn();

	NPCInit();
}

//-----------------------------------------------------------------------------
// Precache - precaches all resources this NPC needs
//-----------------------------------------------------------------------------
void CNPC_Cremator::Precache()
{
	PrecacheModel(STRING(GetModelName()));

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Classify - indicates this NPC's place in the 
// relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_Cremator::Classify(void)
{
	return	CLASS_NONE;
}

int CNPC_Cremator::GetSoundInterests(void)
{
	return NULL;
}
