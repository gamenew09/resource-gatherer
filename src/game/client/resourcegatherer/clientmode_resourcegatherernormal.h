#pragma once

#include "clientmode_shared.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui/Cursor.h>

class CHudViewport;

namespace vgui
{
	typedef unsigned long HScheme;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class ClientModeResourceGathererNormal : public ClientModeShared
{
public:
	DECLARE_CLASS( ClientModeResourceGathererNormal, ClientModeShared );

	ClientModeResourceGathererNormal();
	~ClientModeResourceGathererNormal();

	virtual void	Init();
	virtual int		GetDeathMessageStartHeight( void );

	virtual bool	DoPostScreenSpaceEffects(const CViewSetup *pSetup) OVERRIDE;
};

extern IClientMode *GetClientModeNormal();
extern vgui::HScheme g_hVGuiCombineScheme;

extern ClientModeResourceGathererNormal* GetClientModeResourceGathererNormal();