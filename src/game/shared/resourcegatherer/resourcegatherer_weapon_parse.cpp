#include "cbase.h"
#include <KeyValues.h>
#include "resourcegatherer_weapon_parse.h"
#include "ammodef.h"

FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CResourceGathererWeaponInfo;
}



CResourceGathererWeaponInfo::CResourceGathererWeaponInfo()
{
	m_iPlayerDamage = 0;
}


void CResourceGathererWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

	m_iPlayerDamage = pKeyValuesData->GetInt( "damage", 0 );
}


