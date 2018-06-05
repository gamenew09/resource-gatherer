#pragma once

#include "c_baseanimating.h"

class C_ResourceGathererResourcePickup : public C_BaseAnimating
{
public:
    DECLARE_CLASS(C_ResourceGathererResourcePickup, C_BaseAnimating);
	DECLARE_CLIENTCLASS();

	virtual void OnDataChanged(DataUpdateType_t updateType);
    virtual void Spawn();
	void HandleGlowEffect();
public:
	EResourceType m_eResourceType;
	int m_iWorth;
private:
	int m_hGlow;
};