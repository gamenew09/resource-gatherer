#pragma once

#include "baseanimating.h"

class CResourceGathererResourcePickup : public CBaseAnimating
{
    DECLARE_CLASS( CResourceGathererResourcePickup, CBaseAnimating );
	DECLARE_DATADESC();
public:
	CResourceGathererResourcePickup();
 
	void Spawn();
	void Precache();

	void PlayerCheckerThink();

    void PickupResource(CBasePlayer* pCauser);
	
    EResourceType m_eResourceType;
    int m_iWorth;
private:
	bool m_bHasBeenPickedUp;
};