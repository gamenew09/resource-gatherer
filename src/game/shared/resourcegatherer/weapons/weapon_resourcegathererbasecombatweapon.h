//========= Copyright Valve Corporation, All rights reserved. ============//
#pragma once

#ifdef CLIENT_DLL
	#include "c_resourcegatherer_player.h"
#else
	#include "resourcegatherer_player.h"
#endif

#include "weapon_resourcegathererbase.h"

#if defined( CLIENT_DLL )
#define CBaseResourceGathererCombatWeapon C_BaseResourceGathererCombatWeapon
#endif

class CBaseResourceGathererCombatWeapon : public CWeaponResourceGathererBase
{
#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

	DECLARE_CLASS( CBaseResourceGathererCombatWeapon, CWeaponResourceGathererBase );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CBaseResourceGathererCombatWeapon();

	virtual bool	WeaponShouldBeLowered( void );

	virtual bool	Ready( void );
	virtual bool	Lower( void );
	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	WeaponIdle( void );

	virtual void	AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	virtual	float	CalcViewmodelBob( void );

	virtual Vector	GetBulletSpread( WeaponProficiency_t proficiency );
	virtual float	GetSpreadBias( WeaponProficiency_t proficiency );

	virtual const	WeaponProficiencyInfo_t *GetProficiencyValues();
	static const	WeaponProficiencyInfo_t *GetDefaultProficiencyValues();

	virtual void	ItemHolsterFrame( void );

protected:

	bool			m_bLowered;			// Whether the viewmodel is raised or lowered
	float			m_flRaiseTime;		// If lowered, the time we should raise the viewmodel
	float			m_flHolsterTime;	// When the weapon was holstered

private:
	
	CBaseResourceGathererCombatWeapon( const CBaseResourceGathererCombatWeapon & );
};
