//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef ResourceGatherer_WEAPON_CROWBAR_H
#define ResourceGatherer_WEAPON_CROWBAR_H
#pragma once


#include "weapon_resourcegathererbasecombatweapon.h"
#include "weapon_resourcegathererbasebludgeon.h"


#ifdef CLIENT_DLL
#define CWeaponCrowbar C_WeaponCrowbar
#endif

//-----------------------------------------------------------------------------
// CWeaponCrowbar
//-----------------------------------------------------------------------------

class CWeaponCrowbar : public CBaseResourceGathererBludgeonWeapon
{
public:
	DECLARE_CLASS( CWeaponCrowbar, CBaseResourceGathererBludgeonWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponCrowbar();

	float		GetRange( void );
	float		GetFireRate( void );

	void		AddViewKick( void );
	float		GetDamageForActivity( Activity hitActivity );
	void		SecondaryAttack( void )	{	return;	}

	void		Drop( const Vector &vecVelocity );


	// Animation event
#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	int WeaponMeleeAttack1Condition( float flDot, float flDist );
#endif

	CWeaponCrowbar( const CWeaponCrowbar & );

private:
		
};


#endif // ResourceGatherer_WEAPON_CROWBAR_H

