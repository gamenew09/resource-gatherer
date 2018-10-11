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

//ConVar mp_friendlyfire("mp_friendlyfire", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "");
extern ConVar friendlyfire;

REGISTER_GAMERULES_CLASS( CResourceGathererRules );

BEGIN_NETWORK_TABLE_NOBASE( CResourceGathererRules, DT_ResourceGathererRules )

#ifdef CLIENT_DLL
    RecvPropInt( RECVINFO(m_iBiological) ),
    RecvPropInt( RECVINFO(m_iMechanical) ),
    RecvPropInt( RECVINFO(m_iEnergy) ),
#else
    SendPropInt( SENDINFO(m_iBiological) ),
    SendPropInt( SENDINFO(m_iMechanical) ),
    SendPropInt( SENDINFO(m_iEnergy) ),
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

	CTeam* pSpectatorTeam = static_cast<CTeam*>(CreateEntityByName("team_manager"));
	pSpectatorTeam->Init("Spectator", TEAM_SPECTATOR);

	g_Teams.AddToTail(pUnassignedTeam);
	g_Teams.AddToTail(pSpectatorTeam);
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

	if ((gpGlobals->curtime >= m_flIntermissionEndTime && g_fGameOver) || mp_restartgame_immediate.GetBool())
	{
		mp_restartgame_immediate.SetValue(0);
		mp_restartgame.SetValue(0);
		if (rg_restartmap.GetBool())
		{
			ChangeLevelToMap(STRING(gpGlobals->mapname));
		}
		else
		{
			ChangeLevel();
		}
		return;
	}
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

	if (pKiller->IsPlayer() && pVictim->IsPlayer())
	{
		// Custom kill type?
		if (info.GetDamageCustom())
		{
			killer_weapon_name = GetDamageCustomString(info);
			if (pScorer)
			{
				killer_ID = pScorer->GetUserID();
			}
		}
		else
		{
			// Is the killer a client?
			if (pScorer)
			{
				killer_ID = pScorer->GetUserID();

				if (pInflictor)
				{
					if (pInflictor == pScorer)
					{
						// If the inflictor is the killer,  then it must be their current weapon doing the damage
						if (pScorer->GetActiveWeapon())
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
			if (strncmp(killer_weapon_name, "weapon_", 7) == 0)
			{
				killer_weapon_name += 7;
			}
			else if (strncmp(killer_weapon_name, "npc_", 4) == 0)
			{
				killer_weapon_name += 4;
			}
			else if (strncmp(killer_weapon_name, "func_", 5) == 0)
			{
				killer_weapon_name += 5;
			}
			else if (strstr(killer_weapon_name, "physics"))
			{
				killer_weapon_name = "physics";
			}

			if (strcmp(killer_weapon_name, "prop_combine_ball") == 0)
			{
				killer_weapon_name = "combine_ball";
			}
			else if (strcmp(killer_weapon_name, "grenade_ar2") == 0)
			{
				killer_weapon_name = "smg1_grenade";
			}
			else if (strcmp(killer_weapon_name, "satchel") == 0 || strcmp(killer_weapon_name, "tripmine") == 0)
			{
				killer_weapon_name = "slam";
			}
		}

		IGameEvent *event = gameeventmanager->CreateEvent("player_death");
		if (event)
		{
			event->SetInt("userid", pVictim->GetUserID());
			event->SetInt("attacker", killer_ID);
			event->SetString("weapon", killer_weapon_name);
			event->SetInt("priority", 7);
			gameeventmanager->FireEvent(event);
		}
	}
	else if (pKiller->IsNPC() && pVictim->IsPlayer())
	{
		IGameEvent *event = gameeventmanager->CreateEvent("player_killed_by_npc");
		if (event)
		{
			event->SetInt("userid", pVictim->GetUserID());
			event->SetInt("attacker_npc_entid", pKiller->entindex());
			event->SetString("weapon", killer_weapon_name);
			event->SetInt("priority", 7);
			gameeventmanager->FireEvent(event);
		}
	}

	if (pVictim->IsPlayer()) // Check to see if a player just died.
	{
		bool bShouldStartIntermission = true;

		for (int i = 1; i <= gpGlobals->maxClients; i++) // Iterate through all players.
		{
			CBasePlayer* pBasePly = UTIL_PlayerByIndex(i); // Grab the player entity pointer.
			CResourceGatherer_Player* pPlayer = dynamic_cast<CResourceGatherer_Player*>(pBasePly); // Cast to ResourceGatherer_Player

			if (pBasePly && pPlayer && pPlayer != pVictim) // Check to see if the player we got is valid and the player does not equal to the victim we are currently acting upon.
			{
				if (pPlayer->IsAlive()) // Check to see if the player is alive.
				{
					bShouldStartIntermission = false; // If the player is alive, then we know not to start the intermission.
					break;
				}
				else
				{
					bShouldStartIntermission = true; // Otherwise, we should start the intermission. (? Why did i put this here)
				}
			}
		}

		if (bShouldStartIntermission) // If we should start the intermission.
		{
			GoToIntermission(); // Then go to the intermission phase.
		}
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

	CResourceGatherer_Player *pPlayer = (CResourceGatherer_Player *)pEdict;

	ASSERT(pPlayer);

	if(pPlayer)
	{
		if (pPlayer->ClientCommand(args))
			return true;
	}
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

int CResourceGathererRules::ItemShouldRespawn(CItem *pItem)
{
	return GR_ITEM_RESPAWN_NO;
}

int CResourceGathererRules::WeaponShouldRespawn(CBaseCombatWeapon *pWeapon)
{
	return GR_WEAPON_RESPAWN_NO;
}

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
			pszFormat = "ResourceGatherer_Chat_Spec";
		}
		else
		{
			const char *chatLocation = GetChatLocation( bTeamOnly, pPlayer );
			if ( chatLocation && *chatLocation )
			{
				pszFormat = "ResourceGatherer_Chat_Team_Loc";
			}
			else
			{
				pszFormat = "ResourceGatherer_Chat_Team";
			}
		}
	}
	// everyone
	else
	{
		if ( pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
		{
			pszFormat = "ResourceGatherer_Chat_All";	
		}
		else
		{
			pszFormat = "ResourceGatherer_Chat_AllSpec";
		}
	}

	return pszFormat;
}

bool CResourceGathererRules::TakeResource(CBasePlayer* pCauser, EResourceType eResourceType, int32 iPrice, bool bAlwaysRemoveToZero)
{
	switch (eResourceType)
	{
	case ResourceType_Biological:
		if (m_iBiological < iPrice && !bAlwaysRemoveToZero)
		{
			return false;
		}

		m_iBiological = max(m_iBiological - iPrice, 0);

		return true;
	case ResourceType_Mechanical:
		if (m_iMechanical < iPrice && !bAlwaysRemoveToZero)
		{
			return false;
		}

		m_iMechanical = max(m_iMechanical - iPrice, 0);

		return true;
	case ResourceType_Energy:
		if (m_iEnergy < iPrice && !bAlwaysRemoveToZero)
		{
			return false;
		}

		m_iEnergy = max(m_iEnergy - iPrice, 0);

		return true;
	default:
		Assert(false);
		return false;
	}
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

void CResourceGathererRules::InitDefaultAIRelationships()
{
	int i, j;

	//  Allocate memory for default relationships
	CBaseCombatCharacter::AllocateDefaultRelationships();

	// --------------------------------------------------------------
	// First initialize table so we can report missing relationships
	// --------------------------------------------------------------
	for (i = 0; i<NUM_AI_CLASSES; i++)
	{
		for (j = 0; j<NUM_AI_CLASSES; j++)
		{
			// By default all relationships are neutral of priority zero
			CBaseCombatCharacter::SetDefaultRelationship((Class_T)i, (Class_T)j, D_NU, 0);
		}
	}

	// ------------------------------------------------------------
	//	> CLASS_ANTLION
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_HEADCRAB, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_PROTOSNIPER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_ANTLION, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ANTLION, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_BARNACLE
	//
	//  In this case, the relationship D_HT indicates which characters
	//  the barnacle will try to eat.
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_BARNACLE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_HEADCRAB, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_MANHACK, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_EARTH_FAUNA, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_BULLSEYE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_ANTLION, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_HEADCRAB, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_VORTIGAUNT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_PLAYER_ALLY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_PLAYER_ALLY_VITAL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE, CLASS_HACKED_ROLLERMINE, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_BULLSQUID
	// ------------------------------------------------------------
	/*
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_NONE,				D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PLAYER,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_BARNACLE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_BULLSEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_HEADCRAB,			D_HT, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_HOUNDEYE,			D_HT, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_MANHACK,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_SCANNER,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_STALKER,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_VORTIGAUNT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_HACKED_ROLLERMINE,D_HT, 0);
	*/
	// ------------------------------------------------------------
	//	> CLASS_CITIZEN_PASSIVE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_BARNACLE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_BULLSQUID,		D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_COMBINE_HUNTER, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_HEADCRAB, D_FR, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_HOUNDEYE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_MANHACK, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_MISSILE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_VORTIGAUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_ZOMBIE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_PLAYER_ALLY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_PLAYER_ALLY_VITAL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE, CLASS_HACKED_ROLLERMINE, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_CITIZEN_REBEL
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_BARNACLE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_BULLSQUID,		D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_HEADCRAB, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_MISSILE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_VORTIGAUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_PLAYER_ALLY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_PLAYER_ALLY_VITAL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL, CLASS_HACKED_ROLLERMINE, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_COMBINE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_BARNACLE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_COMBINE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_COMBINE_GUNSHIP, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_COMBINE_HUNTER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_HEADCRAB, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_COMBINE_GUNSHIP
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_COMBINE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_COMBINE_GUNSHIP, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_COMBINE_HUNTER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_HEADCRAB, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_MISSILE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_COMBINE_HUNTER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,	CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_COMBINE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_COMBINE_GUNSHIP, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_COMBINE_HUNTER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_HEADCRAB, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,	CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_CONSCRIPT
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_BARNACLE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_HEADCRAB, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_VORTIGAUNT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_PLAYER_ALLY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_PLAYER_ALLY_VITAL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT, CLASS_HACKED_ROLLERMINE, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_FLARE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_ANTLION, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_HEADCRAB, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_VORTIGAUNT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_PLAYER_ALLY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_PLAYER_ALLY_VITAL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_FLARE, CLASS_HACKED_ROLLERMINE, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_HEADCRAB
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_BULLSQUID,		D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_HEADCRAB, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB, CLASS_HACKED_ROLLERMINE, D_FR, 0);

	// ------------------------------------------------------------
	//	> CLASS_HOUNDEYE
	// ------------------------------------------------------------
	/*
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_NONE,				D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PLAYER,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_ANTLION,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_BARNACLE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_BULLSEYE,			D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_BULLSQUID,		D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_COMBINE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_COMBINE_HUNTER,	D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_CONSCRIPT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_FLARE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_HEADCRAB,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_MANHACK,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_METROPOLICE,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_MILITARY,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_SCANNER,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_STALKER,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_VORTIGAUNT,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PROTOSNIPER,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_EARTH_FAUNA,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);
	*/

	// ------------------------------------------------------------
	//	> CLASS_MANHACK
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_HEADCRAB, D_HT, -1);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_HOUNDEYE,			D_HT,-1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MANHACK, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_METROPOLICE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_HEADCRAB, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_MILITARY
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_HEADCRAB, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MILITARY, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_MISSILE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_HEADCRAB, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_MISSILE, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_NONE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_ANTLION, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_HEADCRAB, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_VORTIGAUNT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_PLAYER_ALLY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_PLAYER_ALLY_VITAL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE, CLASS_HACKED_ROLLERMINE, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_PLAYER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_BARNACLE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_BULLSEYE, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_CITIZEN_PASSIVE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_CITIZEN_REBEL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_COMBINE_GUNSHIP, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_HEADCRAB, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_VORTIGAUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_PROTOSNIPER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_PLAYER_ALLY_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER, CLASS_HACKED_ROLLERMINE, D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER_ALLY
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_BARNACLE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_HEADCRAB, D_FR, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_VORTIGAUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_ZOMBIE, D_FR, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_PROTOSNIPER, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_PLAYER_ALLY_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY, CLASS_HACKED_ROLLERMINE, D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER_ALLY_VITAL
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_BARNACLE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_COMBINE_HUNTER, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_HEADCRAB, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_VORTIGAUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_PROTOSNIPER, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_PLAYER_ALLY_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL, CLASS_HACKED_ROLLERMINE, D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_SCANNER
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_COMBINE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_COMBINE_GUNSHIP, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_COMBINE_HUNTER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_HEADCRAB, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_MANHACK, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_METROPOLICE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_MILITARY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_SCANNER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_STALKER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_PROTOSNIPER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_SCANNER, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_STALKER
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_HEADCRAB, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_STALKER, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_VORTIGAUNT
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_BARNACLE, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_CITIZEN_PASSIVE, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_CITIZEN_REBEL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_HEADCRAB, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_VORTIGAUNT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_PLAYER_ALLY_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT, CLASS_HACKED_ROLLERMINE, D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_ZOMBIE
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_HEADCRAB, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_MANHACK, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_MILITARY, D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_ZOMBIE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_PROTOSNIPER
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_BULLSQUID,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_COMBINE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_HEADCRAB, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_HOUNDEYE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_METROPOLICE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_MILITARY, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_MISSILE, D_NU, 5);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_STALKER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER, CLASS_HACKED_ROLLERMINE, D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_EARTH_FAUNA
	//
	// Hates pretty much everything equally except other earth fauna.
	// This will make the critter choose the nearest thing as its enemy.
	// ------------------------------------------------------------	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_NONE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_PLAYER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_CITIZEN_PASSIVE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_CITIZEN_REBEL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_COMBINE_GUNSHIP, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_COMBINE_HUNTER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_CONSCRIPT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_FLARE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_HEADCRAB, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_MANHACK, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_MISSILE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_SCANNER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_VORTIGAUNT, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_ZOMBIE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_PROTOSNIPER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_EARTH_FAUNA, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_PLAYER_ALLY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_PLAYER_ALLY_VITAL, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA, CLASS_HACKED_ROLLERMINE, D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_HACKED_ROLLERMINE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_NONE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_PLAYER, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_ANTLION, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_BARNACLE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_BULLSEYE, D_NU, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_BULLSQUID,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_CITIZEN_PASSIVE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_CITIZEN_REBEL, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_COMBINE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_COMBINE_GUNSHIP, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_COMBINE_HUNTER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_CONSCRIPT, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_FLARE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_HEADCRAB, D_HT, 0);
	//CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_HOUNDEYE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_MANHACK, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_METROPOLICE, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_MILITARY, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_MISSILE, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_SCANNER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_STALKER, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_VORTIGAUNT, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_ZOMBIE, D_HT, 1);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_PROTOSNIPER, D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_EARTH_FAUNA, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_PLAYER_ALLY, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_PLAYER_ALLY_VITAL, D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE, CLASS_HACKED_ROLLERMINE, D_LI, 0);
}

//=========================================================
//=========================================================
bool CResourceGathererRules::AllowDamage(CBaseEntity *pVictim, const CTakeDamageInfo &info)
{
	return true;
}

//=========================================================
//=========================================================
bool CResourceGathererRules::FPlayerCanTakeDamage(CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info)
{
	if (pAttacker && pAttacker->IsPlayer() && pPlayer != pAttacker && !friendlyfire.GetBool()) // If the attacker is valid, is a player, and isn't damaging themselves, then the victim player should not take damage. RGTODO: Add convar to disable player damage (mp_friendlyfire)
	{
		return false;
	}

	return true;
}

//=========================================================
//=========================================================
void CResourceGathererRules::PlayerThink(CBasePlayer *pPlayer)
{
	BaseClass::PlayerThink(pPlayer);
}

//=========================================================
//=========================================================
void CResourceGathererRules::PlayerSpawn(CBasePlayer *pPlayer)
{
	BaseClass::PlayerSpawn(pPlayer);
}

//=========================================================
//=========================================================
bool CResourceGathererRules::FPlayerCanRespawn(CBasePlayer *pPlayer)
{
	return false;
}

//=========================================================
//=========================================================
float CResourceGathererRules::FlPlayerSpawnTime(CBasePlayer *pPlayer)
{
	return gpGlobals->curtime;//now!
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

#define USE_SKILL_CVARS 1

CAmmoDef *GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if (!bInitted)
	{
		bInitted = true;

#ifdef USE_SKILL_CVARS
		def.AddAmmoType("AR2", DMG_BULLET, TRACER_LINE_AND_WHIZ, "sk_plr_dmg_ar2", "sk_npc_dmg_ar2", "sk_max_ar2", BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("AlyxGun", DMG_BULLET, TRACER_LINE, "sk_plr_dmg_alyxgun", "sk_npc_dmg_alyxgun", "sk_max_alyxgun", BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("Pistol", DMG_BULLET, TRACER_LINE_AND_WHIZ, "sk_plr_dmg_pistol", "sk_npc_dmg_pistol", "sk_max_pistol", BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("SMG1", DMG_BULLET, TRACER_LINE_AND_WHIZ, "sk_plr_dmg_smg1", "sk_npc_dmg_smg1", "sk_max_smg1", BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("357", DMG_BULLET, TRACER_LINE_AND_WHIZ, "sk_plr_dmg_357", "sk_npc_dmg_357", "sk_max_357", BULLET_IMPULSE(800, 5000), 0);
		def.AddAmmoType("XBowBolt", DMG_BULLET, TRACER_LINE, "sk_plr_dmg_crossbow", "sk_npc_dmg_crossbow", "sk_max_crossbow", BULLET_IMPULSE(800, 8000), 0);

		def.AddAmmoType("Buckshot", DMG_BULLET | DMG_BUCKSHOT, TRACER_LINE, "sk_plr_dmg_buckshot", "sk_npc_dmg_buckshot", "sk_max_buckshot", BULLET_IMPULSE(400, 1200), 0);
		def.AddAmmoType("RPG_Round", DMG_BURN, TRACER_NONE, "sk_plr_dmg_rpg_round", "sk_npc_dmg_rpg_round", "sk_max_rpg_round", 0, 0);
		def.AddAmmoType("SMG1_Grenade", DMG_BURN, TRACER_NONE, "sk_plr_dmg_smg1_grenade", "sk_npc_dmg_smg1_grenade", "sk_max_smg1_grenade", 0, 0);
		//def.AddAmmoType("SniperRound", DMG_BULLET | DMG_SNIPER, TRACER_NONE, "sk_plr_dmg_sniper_round", "sk_npc_dmg_sniper_round", "sk_max_sniper_round", BULLET_IMPULSE(650, 6000), 0);
		//def.AddAmmoType("SniperPenetratedRound", DMG_BULLET | DMG_SNIPER, TRACER_NONE, "sk_dmg_sniper_penetrate_plr", "sk_dmg_sniper_penetrate_npc", "sk_max_sniper_round", BULLET_IMPULSE(150, 6000), 0);
		def.AddAmmoType("Grenade", DMG_BURN, TRACER_NONE, "sk_plr_dmg_grenade", "sk_npc_dmg_grenade", "sk_max_grenade", 0, 0);
		def.AddAmmoType("Thumper", DMG_SONIC, TRACER_NONE, 10, 10, 2, 0, 0);
		def.AddAmmoType("Gravity", DMG_CLUB, TRACER_NONE, 0, 0, 8, 0, 0);
		//		def.AddAmmoType("Extinguisher",		DMG_BURN,					TRACER_NONE,			0,	0, 100, 0, 0 );
		def.AddAmmoType("Battery", DMG_CLUB, TRACER_NONE, NULL, NULL, NULL, 0, 0);
		def.AddAmmoType("GaussEnergy", DMG_SHOCK, TRACER_NONE, "sk_jeep_gauss_damage", "sk_jeep_gauss_damage", "sk_max_gauss_round", BULLET_IMPULSE(650, 8000), 0); // hit like a 10kg weight at 400 in/s
		def.AddAmmoType("CombineCannon", DMG_BULLET, TRACER_LINE, "sk_npc_dmg_gunship_to_plr", "sk_npc_dmg_gunship", NULL, 1.5 * 750 * 12, 0); // hit like a 1.5kg weight at 750 ft/s
		def.AddAmmoType("AirboatGun", DMG_AIRBOAT, TRACER_LINE, "sk_plr_dmg_airboat", "sk_npc_dmg_airboat", NULL, BULLET_IMPULSE(10, 600), 0);

		//=====================================================================
		// STRIDER MINIGUN DAMAGE - Pull up a chair and I'll tell you a tale.
		//
		// When we shipped Half-Life 2 in 2004, we were unaware of a bug in
		// CAmmoDef::NPCDamage() which was returning the MaxCarry field of
		// an ammotype as the amount of damage that should be done to a NPC
		// by that type of ammo. Thankfully, the bug only affected Ammo Types 
		// that DO NOT use ConVars to specify their parameters. As you can see,
		// all of the important ammotypes use ConVars, so the effect of the bug
		// was limited. The Strider Minigun was affected, though.
		//
		// According to my perforce Archeology, we intended to ship the Strider
		// Minigun ammo type to do 15 points of damage per shot, and we did. 
		// To achieve this we, unaware of the bug, set the Strider Minigun ammo 
		// type to have a maxcarry of 15, since our observation was that the 
		// number that was there before (8) was indeed the amount of damage being
		// done to NPC's at the time. So we changed the field that was incorrectly
		// being used as the NPC Damage field.
		//
		// The bug was fixed during Episode 1's development. The result of the 
		// bug fix was that the Strider was reduced to doing 5 points of damage
		// to NPC's, since 5 is the value that was being assigned as NPC damage
		// even though the code was returning 15 up to that point.
		//
		// Now as we go to ship Orange Box, we discover that the Striders in 
		// Half-Life 2 are hugely ineffective against citizens, causing big
		// problems in maps 12 and 13. 
		//
		// In order to restore balance to HL2 without upsetting the delicate 
		// balance of ep2_outland_12, I have chosen to build Episodic binaries
		// with 5 as the Strider->NPC damage, since that's the value that has
		// been in place for all of Episode 2's development. Half-Life 2 will
		// build with 15 as the Strider->NPC damage, which is how HL2 shipped
		// originally, only this time the 15 is located in the correct field
		// now that the AmmoDef code is behaving correctly.
		//
		//=====================================================================
#ifdef HL2_EPISODIC
		def.AddAmmoType("StriderMinigun", DMG_BULLET, TRACER_LINE, 5, 5, 15, 1.0 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED); // hit like a 1.0kg weight at 750 ft/s
#else
		def.AddAmmoType("StriderMinigun", DMG_BULLET, TRACER_LINE, 5, 15, 15, 1.0 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED); // hit like a 1.0kg weight at 750 ft/s
#endif//HL2_EPISODIC

		def.AddAmmoType("StriderMinigunDirect", DMG_BULLET, TRACER_LINE, 2, 2, 15, 1.0 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED); // hit like a 1.0kg weight at 750 ft/s
		def.AddAmmoType("HelicopterGun", DMG_BULLET, TRACER_LINE_AND_WHIZ, "sk_npc_dmg_helicopter_to_plr", "sk_npc_dmg_helicopter", "sk_max_smg1", BULLET_IMPULSE(400, 1225), AMMO_FORCE_DROP_IF_CARRIED | AMMO_INTERPRET_PLRDAMAGE_AS_DAMAGE_TO_PLAYER);
		def.AddAmmoType("AR2AltFire", DMG_DISSOLVE, TRACER_NONE, 0, 0, "sk_max_ar2_altfire", 0, 0);
		def.AddAmmoType("Grenade", DMG_BURN, TRACER_NONE, "sk_plr_dmg_grenade", "sk_npc_dmg_grenade", "sk_max_grenade", 0, 0);
#ifdef HL2_EPISODIC
		def.AddAmmoType("Hopwire", DMG_BLAST, TRACER_NONE, "sk_plr_dmg_grenade", "sk_npc_dmg_grenade", "sk_max_hopwire", 0, 0);
		def.AddAmmoType("CombineHeavyCannon", DMG_BULLET, TRACER_LINE, 40, 40, NULL, 10 * 750 * 12, AMMO_FORCE_DROP_IF_CARRIED); // hit like a 10 kg weight at 750 ft/s
		def.AddAmmoType("ammo_proto1", DMG_BULLET, TRACER_LINE, 0, 0, 10, 0, 0);
#endif // HL2_EPISODIC
		def.AddAmmoType("slam", DMG_BURN, TRACER_NONE, 0, 0, 5, 0, 0);
#else
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
#endif
	}

	return &def;
}