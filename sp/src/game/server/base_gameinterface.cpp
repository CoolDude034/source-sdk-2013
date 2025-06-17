//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "gameinterface.h"
#include "mapentities.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void CServerGameClients::GetPlayerLimits( int& minplayers, int& maxplayers, int &defaultMaxPlayers ) const
{
	minplayers = defaultMaxPlayers = 1; 
	maxplayers = MAX_PLAYERS;
}


// -------------------------------------------------------------------------------------------- //
// Mod-specific CServerGameDLL implementation.
// -------------------------------------------------------------------------------------------- //

ConVar difficulty("difficulty", "2");

void CServerGameDLL::LevelInit_ParseAllEntities( const char *pMapEntities )
{
	switch (difficulty.GetInt())
	{
	case 1:
		g_iSkillLevel = SKILL_EASY;
		break;
	case 2:
		g_iSkillLevel = SKILL_MEDIUM;
		break;
	case 3:
		g_iSkillLevel = SKILL_HARD;
		break;
	case 4:
		g_iSkillLevel = SKILL_VERY_HARD;
		break;
	case 5:
		g_iSkillLevel = SKILL_NIGHTMARE;
		break;
	default:
		g_iSkillLevel = SKILL_MEDIUM;
		break;
	}

	DevMsg("Difficulty set!\n");
}
