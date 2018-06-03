#include "cbase.h"
#include "resourcegatherer_gamerules.h"

#include "viewport_panel_names.h"
#include "gameeventdefs.h"
#include <KeyValues.h>
#include <convar.h>
#include "ammodef.h"

#ifdef CLIENT_DLL
#include "c_resourcegatherer_player.h"
#else
#include "eventqueue.h"
#include "player.h"
#include "gamerules.h"
#include "game.h"
#include "items.h"
#include "entitylist.h"
#include "mapentities.h"
#include "in_buttons.h"
#include <ctype.h>
#include "voice_gamemgr.h"
#include "iscorer.h"
#include "resourcegatherer_player.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "team.h"
#include "voice_gamemgr.h"
#include "resourcegatherer_gameinterface.h"
#include "resourcegatherer_cvars.h"

#include "rg_resourcepickup.h"
#endif

REGISTER_GAMERULES_CLASS( CResourceGathererRules );

BEGIN_NETWORK_TABLE_NOBASE( CResourceGathererRules, DT_ResourceGathererRules )

#ifdef CLIENT_DLL
    RecvPropInt( RECVINFO(m_iBiological) ),
    RecvPropInt( RECVINFO(m_iMechanical) ),
    RecvPropInt( RECVINFO(m_iEnergy) ),
#else
    SendPropInt( SENDINFO(m_iBiological), 8, SPROP_CHANGES_OFTEN ),
    SendPropInt( SENDINFO(m_iMechanical), 8, SPROP_CHANGES_OFTEN ),
    SendPropInt( SENDINFO(m_iEnergy), 8, SPROP_CHANGES_OFTEN ),
#endif

END_NETWORK_TABLE()

static ResourceGathererViewVectors g_RGViewVectors(
	Vector( 0, 0, 64 ),       //VEC_VIEW (m_vView) 
							  
	Vector(-16, -16, 0 ),	  //VEC_HULL_MIN (m_vHullMin)
	Vector( 16,  16,  72 ),	  //VEC_HULL_MAX (m_vHullMax)
							  					
	Vector(-16, -16, 0 ),	  //VEC_DUCK_HULL_MIN (m_vDuckHullMin)
	Vector( 16,  16,  36 ),	  //VEC_DUCK_HULL_MAX	(m_vDuckHullMax)
	Vector( 0, 0, 28 ),		  //VEC_DUCK_VIEW		(m_vDuckView)
							  					
	Vector(-10, -10, -10 ),	  //VEC_OBS_HULL_MIN	(m_vObsHullMin)
	Vector( 10,  10,  10 ),	  //VEC_OBS_HULL_MAX	(m_vObsHullMax)
							  					
	Vector( 0, 0, 14 ),		  //VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)

	Vector(-16, -16, 0 ),	  //VEC_CROUCH_TRACE_MIN (m_vCrouchTraceMin)
	Vector( 16,  16,  60 )	  //VEC_CROUCH_TRACE_MAX (m_vCrouchTraceMax)
);

LINK_ENTITY_TO_CLASS( resourcegatherer_gamerules, CResourceGathererRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( ResourceGathererRulesProxy, DT_ResourceGathererRulesProxy )

#ifdef CLIENT_DLL
	void RecvProxy_ResourceGathererRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CResourceGathererRules *pRules = ResourceGathererRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CResourceGathererRulesProxy, DT_ResourceGathererRulesProxy )
		RecvPropDataTable( "hl2mp_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_ResourceGathererRules ), RecvProxy_ResourceGathererRules )
	END_RECV_TABLE()
#else
	void* SendProxy_ResourceGathererRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CResourceGathererRules *pRules = ResourceGathererRules();
		Assert( pRules );
		return pRules;
	}

	BEGIN_SEND_TABLE( CResourceGathererRulesProxy, DT_ResourceGathererRulesProxy )
		SendPropDataTable( "hl2mp_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_ResourceGathererRules ), SendProxy_ResourceGathererRules )
	END_SEND_TABLE()
#endif

#ifndef CLIENT_DLL

	class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
	{
	public:
		virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
		{
			return true; // Currently everyone can hear each other.
		}
	};
	CVoiceGameMgrHelper g_VoiceGameMgrHelper;
	IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

#endif

CResourceGathererRules::CResourceGathererRules()
{
#ifndef CLIENT_DLL
	m_iBiological = m_iMechanical = m_iEnergy = 0;

	CTeam* pUnassignedTeam = static_cast<CTeam*>(CreateEntityByName("team_manager"));
	pUnassignedTeam->Init("Unassigned", TEAM_UNASSIGNED);

	g_Teams.AddToTail(pUnassignedTeam);
#endif
}

CResourceGathererRules::~CResourceGathererRules( void )
{
#ifndef CLIENT_DLL
	g_Teams.Purge();
#endif
}

const char *CResourceGathererRules::GetGameDescription( void )
{ 
	return "Resource Gatherer";
} 

void CResourceGathererRules::Precache()
{

}

void CResourceGathererRules::Think( void )
{
#ifndef CLIENT_DLL
	CGameRules::Think();
#endif
}

#ifdef CLIENT_DLL
void RG_TellCount(void)
{
	if (ResourceGathererRules())
	{
		Msg("Biological: %i\n", ResourceGathererRules()->GetBiologicalResourceCount());
		Msg("Mechanical: %i\n", ResourceGathererRules()->GetMechanicalResourceCount());
		Msg("Energy: %i\n", ResourceGathererRules()->GetEnergyResourceCount());
	}
}

ConCommand rg_tellresources("rg_tellresources", RG_TellCount, "Shows a message.", 0);
#endif

//=========================================================
// Deathnotice. 
//=========================================================
void CResourceGathererRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
#ifndef CLIENT_DLL
	// Work out what killed the player, and send a message to all clients about it
	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_ID = 0;

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor );

	// Custom kill type?
	if ( info.GetDamageCustom() )
	{
		killer_weapon_name = GetDamageCustomString( info );
		if ( pScorer )
		{
			killer_ID = pScorer->GetUserID();
		}
	}
	else
	{
		// Is the killer a client?
		if ( pScorer )
		{
			killer_ID = pScorer->GetUserID();
			
			if ( pInflictor )
			{
				if ( pInflictor == pScorer )
				{
					// If the inflictor is the killer,  then it must be their current weapon doing the damage
					if ( pScorer->GetActiveWeapon() )
					{
						killer_weapon_name = pScorer->GetActiveWeapon()->GetClassname();
					}
				}
				else
				{
					killer_weapon_name = pInflictor->GetClassname();  // it's just that easy
				}
			}
		}
		else
		{
			killer_weapon_name = pInflictor->GetClassname();
		}

		// strip the NPC_* or weapon_* from the inflictor's classname
		if ( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
		{
			killer_weapon_name += 7;
		}
		else if ( strncmp( killer_weapon_name, "npc_", 4 ) == 0 )
		{
			killer_weapon_name += 4;
		}
		else if ( strncmp( killer_weapon_name, "func_", 5 ) == 0 )
		{
			killer_weapon_name += 5;
		}
		else if ( strstr( killer_weapon_name, "physics" ) )
		{
			killer_weapon_name = "physics";
		}

		if ( strcmp( killer_weapon_name, "prop_combine_ball" ) == 0 )
		{
			killer_weapon_name = "combine_ball";
		}
		else if ( strcmp( killer_weapon_name, "grenade_ar2" ) == 0 )
		{
			killer_weapon_name = "smg1_grenade";
		}
		else if ( strcmp( killer_weapon_name, "satchel" ) == 0 || strcmp( killer_weapon_name, "tripmine" ) == 0)
		{
			killer_weapon_name = "slam";
		}


	}

	IGameEvent *event = gameeventmanager->CreateEvent( "player_death" );
	if( event )
	{
		event->SetInt("userid", pVictim->GetUserID() );
		event->SetInt("attacker", killer_ID );
		event->SetString("weapon", killer_weapon_name );
		event->SetInt( "priority", 7 );
		gameeventmanager->FireEvent( event );
	}
#endif

}

bool CResourceGathererRules::IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer )
{ 
	return true; 
}

int CResourceGathererRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
    return GR_TEAMMATE;
}

bool CResourceGathererRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		V_swap(collisionGroup0,collisionGroup1);
	}

	if ( (collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
		collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		return false;
	}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 

}

void CResourceGathererRules::CreateStandardEntities( void )
{

#ifndef CLIENT_DLL
	// Create the entity that will send our data to the client.

	BaseClass::CreateStandardEntities();

#ifdef DBGFLAG_ASSERT
	CBaseEntity *pEnt = 
#endif
	CBaseEntity::Create( "resourcegatherer_gamerules", vec3_origin, vec3_angle );
	Assert( pEnt );
#endif
}

bool CResourceGathererRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
#ifndef CLIENT_DLL
	if( BaseClass::ClientCommand( pEdict, args ) )
		return true;

	/*
	CHL2MP_Player *pPlayer = (CHL2MP_Player *) pEdict;

	if ( pPlayer->ClientCommand( args ) )
		return true;
	*/
#endif

	return false;
}

const CViewVectors* CResourceGathererRules::GetViewVectors() const
{
	return &g_RGViewVectors;
}

const ResourceGathererViewVectors* CResourceGathererRules::GetRGViewVectors() const
{
	return &g_RGViewVectors;
}

bool CResourceGathererRules::CanBuyItem(CBasePlayer* pPlayer, EResourceType eResourceType, int32 iPrice)
{
	switch (eResourceType)
	{
	case ResourceType_Biological:
		return m_iBiological >= iPrice;
	case ResourceType_Mechanical:
		return m_iMechanical >= iPrice;
	case ResourceType_Energy:
		return m_iEnergy >= iPrice;
	default:
		return false;
	}
}

#ifndef CLIENT_DLL
CResourceGathererResourcePickup* CResourceGathererRules::CreateResourcePickup(EResourceType eResourceType, int iWorth)
{
	CResourceGathererResourcePickup *pEnt = dynamic_cast<CResourceGathererResourcePickup*>(CreateEntityByName("rg_resourcepickup"));
	if (pEnt)
	{
		pEnt->m_eResourceType = eResourceType;
		pEnt->m_iWorth = iWorth;

		return pEnt;
	}
	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CResourceGathererRules::GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if ( !pPlayer )  // dedicated server output
	{
		return NULL;
	}

	const char *pszFormat = NULL;

	// team only
	if ( bTeamOnly == TRUE )
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "HL2MP_Chat_Spec";
		}
		else
		{
			const char *chatLocation = GetChatLocation( bTeamOnly, pPlayer );
			if ( chatLocation && *chatLocation )
			{
				pszFormat = "HL2MP_Chat_Team_Loc";
			}
			else
			{
				pszFormat = "HL2MP_Chat_Team";
			}
		}
	}
	// everyone
	else
	{
		if ( pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
		{
			pszFormat = "HL2MP_Chat_All";	
		}
		else
		{
			pszFormat = "HL2MP_Chat_AllSpec";
		}
	}

	return pszFormat;
}

bool CResourceGathererRules::TakeResource(CBasePlayer* pCauser, EResourceType eResourceType, int32 iPrice)
{
	return true; // DEBUG: For now
}

void CResourceGathererRules::AddResource(CBasePlayer* pCauser, EResourceType eResourceType, int32 iAmt)
{
	switch (eResourceType)
	{
	case ResourceType_Biological:
		m_iBiological += iAmt;
		break;
	case ResourceType_Mechanical:
		m_iMechanical += iAmt;
		break;
	case ResourceType_Energy:
		m_iEnergy += iAmt;
		break;
	default:
		Assert(false); // We should never have an invalid value for adding to resource count. Assert and make sure it is known.
		return;
	}

	IGameEvent *event = gameeventmanager->CreateEvent("resource_added");
	if (event)
	{
		event->SetInt("causer", pCauser->GetUserID());
		event->SetInt("resource_type", eResourceType);
		event->SetInt("amount", iAmt);
		gameeventmanager->FireEvent(event);
	}
}

#endif

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			3.5
// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef *GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if (!bInitted)
	{
		bInitted = true;

		def.AddAmmoType("AR2", DMG_BULLET, TRACER_LINE_AND_WHIZ, 0, 0, 60, BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("AR2AltFire", DMG_DISSOLVE, TRACER_NONE, 0, 0, 3, 0, 0);
		def.AddAmmoType("Pistol", DMG_BULLET, TRACER_LINE_AND_WHIZ, 0, 0, 150, BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("SMG1", DMG_BULLET, TRACER_LINE_AND_WHIZ, 0, 0, 225, BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("357", DMG_BULLET, TRACER_LINE_AND_WHIZ, 0, 0, 12, BULLET_IMPULSE(800, 5000), 0);
		def.AddAmmoType("XBowBolt", DMG_BULLET, TRACER_LINE, 0, 0, 10, BULLET_IMPULSE(800, 8000), 0);
		def.AddAmmoType("Buckshot", DMG_BULLET | DMG_BUCKSHOT, TRACER_LINE, 0, 0, 30, BULLET_IMPULSE(400, 1200), 0);
		def.AddAmmoType("RPG_Round", DMG_BURN, TRACER_NONE, 0, 0, 3, 0, 0);
		def.AddAmmoType("SMG1_Grenade", DMG_BURN, TRACER_NONE, 0, 0, 3, 0, 0);
		def.AddAmmoType("Grenade", DMG_BURN, TRACER_NONE, 0, 0, 5, 0, 0);
		def.AddAmmoType("slam", DMG_BURN, TRACER_NONE, 0, 0, 5, 0, 0);
	}

	return &def;
}