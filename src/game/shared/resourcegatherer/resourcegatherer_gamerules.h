#pragma once

#include "gamerules.h"
#include "multiplay_gamerules.h"
#include "gamevars_shared.h"

#ifndef CLIENT_DLL
#include "resourcegatherer_player.h"
#endif

#define VEC_CROUCH_TRACE_MIN	ResourceGathererRules()->GetRGViewVectors()->m_vCrouchTraceMin
#define VEC_CROUCH_TRACE_MAX	ResourceGathererRules()->GetRGViewVectors()->m_vCrouchTraceMax

#ifdef CLIENT_DLL
	#define CResourceGathererRules C_ResourceGathererRules
	#define CResourceGathererRulesProxy C_ResourceGathererRulesProxy
#endif

enum EResourceType
{
    ResourceType_None = 0,
    ResourceType_Biological,
    ResourceType_Mechanical,
    ResourceType_Energy,

	ResourceType_Count
};

inline const char* ResourceTypeAsString(EResourceType eResourceType)
{
	switch (eResourceType)
	{
	case ResourceType_Biological:
		return "Biological";
	case ResourceType_Mechanical:
		return "Mechanical";
	case ResourceType_Energy:
		return "Energy";
	default:
		return "None";
	}
}

class CResourceGathererResourcePickup;

class CResourceGathererRulesProxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS( CResourceGathererRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class ResourceGathererViewVectors : public CViewVectors
{
public:
	ResourceGathererViewVectors( 
		Vector vView,
		Vector vHullMin,
		Vector vHullMax,
		Vector vDuckHullMin,
		Vector vDuckHullMax,
		Vector vDuckView,
		Vector vObsHullMin,
		Vector vObsHullMax,
		Vector vDeadViewHeight,
		Vector vCrouchTraceMin,
		Vector vCrouchTraceMax ) :
			CViewVectors( 
				vView,
				vHullMin,
				vHullMax,
				vDuckHullMin,
				vDuckHullMax,
				vDuckView,
				vObsHullMin,
				vObsHullMax,
				vDeadViewHeight )
	{
		m_vCrouchTraceMin = vCrouchTraceMin;
		m_vCrouchTraceMax = vCrouchTraceMax;
	}

	Vector m_vCrouchTraceMin;
	Vector m_vCrouchTraceMax;
};

class CResourceGathererRules : public CMultiplayRules
{
public:
    DECLARE_CLASS( CResourceGathererRules, CMultiplayRules );
#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars
#else
	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.
#endif

    CResourceGathererRules();
    virtual ~CResourceGathererRules();

	virtual bool IsMultiplayer( void ) { return true; }

	virtual bool IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer );

    virtual void CreateStandardEntities();

    virtual const char* GetGameDescription();

    virtual void Precache();

    virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );

    virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );

    virtual void Think();

    virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );

    int GetBiologicalResourceCount() const { return m_iBiological; }
    int GetMechanicalResourceCount() const { return m_iMechanical; }
    int GetEnergyResourceCount() const { return m_iEnergy; }

    virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );

    virtual bool CanBuyItem(CBasePlayer* pPlayer, EResourceType eResourceType, int32 iPrice); // Should we use item instead?
#ifndef CLIENT_DLL
    virtual bool TakeResource(CBasePlayer* pCauser, EResourceType eResourceType, int32 iPrice);

	virtual void AddResource(CBasePlayer* pCauser, EResourceType eResourceType, int32 iAmt);

    const char *GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer );

    virtual CResourceGathererResourcePickup* CreateResourcePickup(EResourceType eResourceType, int iWorth);
#endif

    virtual const ResourceGathererViewVectors* GetRGViewVectors() const;
    virtual const CViewVectors* GetViewVectors() const;
private:
    // Resource Counters
    CNetworkVar( int32, m_iBiological );
    CNetworkVar( int32, m_iMechanical );
    CNetworkVar( int32, m_iEnergy );
};

inline CResourceGathererRules* ResourceGathererRules()
{
    return static_cast<CResourceGathererRules*>(g_pGameRules);
}