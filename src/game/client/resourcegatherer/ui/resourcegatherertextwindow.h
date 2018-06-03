//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CSTEXTWINDOW_H
#define CSTEXTWINDOW_H
#ifdef _WIN32
#pragma once
#endif

#include "vguitextwindow.h"
#include <spectatorgui.h>

//-----------------------------------------------------------------------------
// Purpose: displays the MOTD
//-----------------------------------------------------------------------------

class CResourceGathererTextWindow : public CTextWindow
{
private:
	DECLARE_CLASS_SIMPLE( CResourceGathererTextWindow, CTextWindow );

public:
	CResourceGathererTextWindow(IViewPort *pViewPort);
	virtual ~CResourceGathererTextWindow();

	virtual void Update();
	virtual void SetVisible(bool state);
	virtual void ShowPanel( bool bShow );
	virtual void OnKeyCodePressed(vgui::KeyCode code);

protected:
	ButtonCode_t m_iScoreBoardKey;

	// Background panel -------------------------------------------------------

public:
	virtual void PaintBackground();
	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	bool m_backgroundLayoutFinished;

	// End background panel ---------------------------------------------------
};

class CResourceGathererSpectatorGUI : public CSpectatorGUI
{
private:
	DECLARE_CLASS_SIMPLE( CResourceGathererSpectatorGUI, CSpectatorGUI );

public:
	CResourceGathererSpectatorGUI( IViewPort *pViewPort );

	virtual void Update( void );
	virtual bool NeedsUpdate( void );

protected:
	int		m_nLastSpecMode;
	CBaseEntity	*m_nLastSpecTarget;
};



#endif // CSTEXTWINDOW_H
