//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is a timer that will forever count upward in HH:MM:SS format
//
// $NoKeywords: $
//
//=============================================================================//
//
// Timer.cpp
//
// implementation of CHudSpeedrunTimer class
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

//-----------------------------------------------------------------------------
// Purpose: Timer panel
//-----------------------------------------------------------------------------
class CHudSpeedrunTimer : public CHudElement, public CHudBaseTimer
{
	DECLARE_CLASS_SIMPLE(CHudSpeedrunTimer, CHudBaseTimer);

public:
	CHudSpeedrunTimer(const char* pElementName);
	virtual void Init(void);
	virtual void VidInit(void);
	virtual void Reset(void);
	virtual void OnThink();
	virtual void Paint();
	void StartTimer();
	bool IsEnabled();

private:
	float m_fStartTime;
	float totalSeconds;
	bool m_Enabled;
};

DECLARE_HUDELEMENT(CHudSpeedrunTimer);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudSpeedrunTimer::CHudSpeedrunTimer(const char* pElementName) : CHudElement(pElementName), CHudBaseTimer(NULL, "HudSpeedrunTimer")
{
}

void CHudSpeedrunTimer::StartTimer()
{
	m_Enabled = true;
	m_fStartTime = gpGlobals->realtime;
	SetPaintEnabled(true);
}

bool CHudSpeedrunTimer::IsEnabled()
{
	return m_Enabled;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSpeedrunTimer::Init()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSpeedrunTimer::Reset()
{
	m_Enabled = false;
	m_fStartTime = 0;
	SetPaintEnabled(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSpeedrunTimer::VidInit()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSpeedrunTimer::Paint()
{
	int total_seconds = static_cast<int>(totalSeconds);
	int hours = total_seconds / 3600;
	int minutes = (total_seconds % 3600) / 60;
	int seconds = total_seconds % 60;
	wchar_t unicode[16];
	swprintf(unicode, L"%02d:%02d:%02d", hours, minutes, seconds);

	// TODO: customize the font later, for now just basic UI will do
	// CreateFont() ignores .res file stuff so we need to customize the UI here within the code
	HFont font = surface()->CreateFont();
	surface()->DrawSetTextFont(font);
	surface()->DrawSetTextColor(255, 255, 255, 255); // white
	surface()->DrawSetTextPos(10, 10); // top-left corner
	surface()->DrawPrintText(unicode, wcslen(unicode));

	for (wchar_t* ch = unicode; *ch != 0; ch++)
	{
		surface()->DrawUnicodeChar(*ch);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSpeedrunTimer::OnThink()
{
	if (!m_Enabled) return;
	totalSeconds = gpGlobals->realtime - m_fStartTime;
}

CON_COMMAND(roundtimer_start, "Start the round timer (should prob set the duration first before calling this)")
{
	CHudSpeedrunTimer* pHud = GET_HUDELEMENT(CHudSpeedrunTimer);
	if (!pHud)
		return;

	if (pHud->IsEnabled())
	{
		pHud->Reset();
	}
	else
	{
		pHud->StartTimer();
	}
}
