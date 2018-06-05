#include "cbase.h"

#include "resourcegatherer_gamerules.h"
#include "c_rg_resourcepickup.h"
#include "glow_outline_effect.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined(CResourceGathererResourcePickup)
#undef CResourceGathererResourcePickup
#endif

LINK_ENTITY_TO_CLASS(rg_resourcepickup, C_ResourceGathererResourcePickup);

IMPLEMENT_CLIENTCLASS_DT(C_ResourceGathererResourcePickup, DT_ResourceGathererResourcePickup, CResourceGathererResourcePickup)
    RecvPropInt(RECVINFO(m_eResourceType)),
	RecvPropInt(RECVINFO(m_iWorth)),
END_RECV_TABLE()

void C_ResourceGathererResourcePickup::Spawn()
{
	BaseClass::Spawn();

    //HandleGlowEffect();
}

void C_ResourceGathererResourcePickup::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	HandleGlowEffect();
}

void C_ResourceGathererResourcePickup::HandleGlowEffect()
{
	if (m_eResourceType >= ResourceType_Biological && m_eResourceType < ResourceType_Count)
	{
		// Checks if the object is already glowing.
		if (m_hGlow)
			return;

		Vector color = Vector(0, 0, 0);

		switch (m_eResourceType)
		{
		case ResourceType_Biological:
			color = Vector(0, 255, 0);
			break;
		case ResourceType_Mechanical:
			color = Vector(255, 255, 0);
			break;
		case ResourceType_Energy:
			color = Vector(0, 0, 255);
			break;
		}

		// Creates a glow object for this entity.
		m_hGlow = g_GlowObjectManager.RegisterGlowObject(this, color, 1.0f, false, true, 0);
	}
}
