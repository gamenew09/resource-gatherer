#pragma once

#include "baseanimating.h"

class CResourceGathererResourcePickup : public CBaseAnimating
{
    DECLARE_CLASS( CResourceGathererResourcePickup, CBaseAnimating );
	DECLARE_SERVERCLASS();

	DECLARE_DATADESC();
public:
	CResourceGathererResourcePickup();
 
	void Spawn();
	void Precache();

	int UpdateTransmitState()	// always send to all clients
	{
		return SetTransmitState(FL_EDICT_ALWAYS);
	}

	void PlayerCheckerThink();

    void PickupResource(CBasePlayer* pCauser);
	
    CNetworkVar(EResourceType, m_eResourceType);
    CNetworkVar(int, m_iWorth);

	COutputEvent m_OnPickedUp;
private:
	bool m_bHasBeenPickedUp;
};