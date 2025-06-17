//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// Health.cpp
//
// implementation of CHudHealth class
//
#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"

#include "iclientmode.h"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>

#include <vgui/ILocalize.h>

using namespace vgui;

#include "hudelement.h"
#include "hud_basetimer.h"

#include "convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_timer_start("cl_timer_start", "0");
ConVar cl_timer_duration("cl_timer_duration", "60");
ConVar cl_timer_should_start("cl_timer_should_start", "0", FCVAR_HIDDEN);

//-----------------------------------------------------------------------------
// Purpose: Health panel
//-----------------------------------------------------------------------------
class CHudRoundTimer : public CHudElement, public CHudBaseTimer
{
	DECLARE_CLASS_SIMPLE(CHudRoundTimer, CHudBaseTimer);

public:
	CHudRoundTimer(const char* pElementName);
	virtual void Init(void);
	virtual void VidInit(void);
	virtual void Reset(void);
	virtual void OnThink();
	void PaintTime(HFont font, int xpos, int ypos, int mins, int secs);

	int GetRoundtimerRemain();
	void StartRoundtimer(int iDuration);

private:
	// old variables
	float m_fStart;
	int m_iDuration;
};

DECLARE_HUDELEMENT(CHudRoundTimer);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudRoundTimer::CHudRoundTimer(const char* pElementName) : CHudElement(pElementName), CHudBaseTimer(NULL, "HudRoundtimer")
{
}

int CHudRoundTimer::GetRoundtimerRemain() {
	return m_fStart < 0 ? -1 : m_iDuration - int(gpGlobals->frametime - m_fStart);
}

void CHudRoundTimer::StartRoundtimer(int iDuration) {
	m_iDuration = iDuration;
	m_fStart = gpGlobals->frametime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudRoundTimer::Init()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudRoundTimer::Reset()
{
	m_iDuration = 0;
	m_fStart = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudRoundTimer::VidInit()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudRoundTimer::PaintTime(HFont font, int xpos, int ypos, int mins, int secs)
{
	int iSeconds = mins * 60 + secs;
	surface()->DrawSetTextFont(font);
	wchar_t unicode[6];
	swprintf(unicode, L"%02dM%02d", iSeconds/60, iSeconds%60);

	surface()->DrawSetTextPos(xpos, ypos);
	for (wchar_t* ch = unicode; *ch != 0; ch++)
	{
		surface()->DrawUnicodeChar(*ch);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudRoundTimer::OnThink()
{
	if (cl_timer_should_start.GetBool())
	{
		StartRoundtimer(cl_timer_duration.GetInt());
		cl_timer_should_start.SetValue(0);
		return;
	}
	int iRemain = GetRoundtimerRemain();
	// There was no timer and still is no timer or there's no new time to display.
	if ((iRemain < 0 && m_iDuration < 0) || (iRemain == m_iDuration))
		return;

	// If we're here, there's was a timer before, but if's it's not there anymore, we need to hide it.
	if (iRemain < 0) {
		SetPaintEnabled(false);
		SetPaintBackgroundEnabled(false);
		m_iDuration = -1;
		return;
	}

	// There was no timer before or the timer has been restarted.
	if ((m_fStart != 0) || m_iDuration < 0) {
		// start the init-event to reset the changing properties
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("RoundtimerInit");
		SetPaintEnabled(true);
		SetPaintBackgroundEnabled(true);
		// save starttime to detect a timer-restart
		m_fStart = cl_timer_start.GetFloat();
	}

	// When the timer reaches 20, change the color to red.
	if (m_iDuration == 20)
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("RoundtimerBelow20");
	else
		// For every time below 10 make it pulse.
		if (iRemain < 10)
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("RoundtimerPulse");

	// Move it down for the last 5 seconds. 
	if (iRemain == 5)
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("RoundtimerBelow5");
	m_iDuration = iRemain;
	SetSeconds(m_iDuration);
}

CON_COMMAND(roundtimer_start, "Start the round timer (should prob set the duration first before calling this)")
{
	// do nothing if there is not exactly one argument
	if (cl_timer_duration.GetInt() <= 0)
		return;

	// evaluate the argument to an integer and start the timer with it
	cl_timer_should_start.SetValue(1);
}