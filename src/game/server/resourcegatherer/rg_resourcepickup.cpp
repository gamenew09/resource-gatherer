#include "cbase.h"

#include "resourcegatherer_gamerules.h"
#include "rg_resourcepickup.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(rg_resourcepickup, CResourceGathererResourcePickup);
 
// Start of our data description for the class
BEGIN_DATADESC( CResourceGathererResourcePickup )
 
	// Save/restore our active state
	DEFINE_FIELD( m_eResourceType, FIELD_INTEGER ),
	DEFINE_FIELD( m_iWorth, FIELD_INTEGER ),
 
END_DATADESC()

CResourceGathererResourcePickup::CResourceGathererResourcePickup()
{
	m_bHasBeenPickedUp = false;
	m_eResourceType = ResourceType_None;
	m_iWorth = 0;
}

#define ENERGY_DROP_MODEL "models/props_hive/larval_essence.mdl"
#define MECHANICAL_DROP_MODEL "models/gibs/manhack_gib01.mdl"
#define BIOLOGICAL_DROP_MODEL "models/weapons/w_bugbait.mdl"

// RGTODO: Convert Pickup Sounds to sound entries. "ResourcePickup.Energy" for instance.

#define ENERGY_PICKUP_SOUND "Resource.Pickup.Energy"
#define MECHANICAL_PICKUP_SOUND "Resource.Pickup.Mechanical"
#define BIOLOGICAL_PICKUP_SOUND "Resource.Pickup.Biological"

void CResourceGathererResourcePickup::PickupResource(CBasePlayer* pCauser)
{
    if(ResourceGathererRules() && pCauser && !m_bHasBeenPickedUp)
    {
		switch (m_eResourceType)
		{
		case ResourceType_Biological:
			EmitSound(BIOLOGICAL_PICKUP_SOUND);
			break;
		case ResourceType_Mechanical:
			EmitSound(MECHANICAL_PICKUP_SOUND);
			break;
		case ResourceType_Energy:
			EmitSound(ENERGY_PICKUP_SOUND);
			break;
		default:
			Assert(false);
			EmitSound(BIOLOGICAL_PICKUP_SOUND);
			break;
		}
		ResourceGathererRules()->AddResource(pCauser, m_eResourceType, m_iWorth);
		m_bHasBeenPickedUp = true;
		UTIL_Remove(this); // Make sure that the resource is removed since it has been picked up.
    }
}

void CResourceGathererResourcePickup::Precache()
{
	switch (m_eResourceType)
	{
	case ResourceType_Biological:
		PrecacheModel(BIOLOGICAL_DROP_MODEL);
		PrecacheScriptSound(BIOLOGICAL_PICKUP_SOUND);
		break;
	case ResourceType_Mechanical:
		PrecacheModel(MECHANICAL_DROP_MODEL);
		PrecacheScriptSound(MECHANICAL_PICKUP_SOUND);
		break;
	case ResourceType_Energy:
		PrecacheModel(ENERGY_DROP_MODEL);
		PrecacheScriptSound(ENERGY_PICKUP_SOUND);
		break;
	default:
		Assert(false);
		PrecacheModel(BIOLOGICAL_DROP_MODEL);
		PrecacheScriptSound(BIOLOGICAL_PICKUP_SOUND);
		break;
	}
 
	BaseClass::Precache();
}


void CResourceGathererResourcePickup::PlayerCheckerThink()
{
	if (!m_bHasBeenPickedUp)
	{
		Vector prop_mins, prop_maxs;
		CollisionProp()->WorldSpaceSurroundingBounds(&prop_mins, &prop_maxs);

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBasePlayer* pPlayer = UTIL_PlayerByIndex(i);
			if (!pPlayer || !pPlayer->IsPlayer())
				continue;

			Vector ply_mins, ply_maxs;

			pPlayer->CollisionProp()->WorldSpaceSurroundingBounds(&ply_mins, &ply_maxs);

			if ((prop_mins.x <= ply_maxs.x && prop_maxs.x >= ply_mins.x &&
				prop_mins.y <= ply_maxs.y && prop_maxs.y >= ply_mins.y &&
				prop_mins.z <= ply_maxs.z && prop_maxs.z >= ply_mins.z))
			{
				PickupResource(pPlayer);
				break;
			}
		}

		SetNextThink(gpGlobals->curtime + 0.1f);
	}
}

void CResourceGathererResourcePickup::Spawn()
{
	Precache();
 
	SetCollisionGroup(COLLISION_GROUP_DEBRIS);

	SetThink(&CResourceGathererResourcePickup::PlayerCheckerThink);
	SetNextThink(gpGlobals->curtime + 0.1f); // Think in .1 seconds. 100 ms

	switch (m_eResourceType)
	{
	case ResourceType_Biological:
		SetModel(BIOLOGICAL_DROP_MODEL);
		break;
	case ResourceType_Mechanical:
		SetModel(MECHANICAL_DROP_MODEL);
		break;
	case ResourceType_Energy:
		SetModel(ENERGY_DROP_MODEL);
		break;
	default:
		Assert(false);
		SetModel(BIOLOGICAL_DROP_MODEL);
		break;
	}
	
	VPhysicsInitNormal(SOLID_VPHYSICS, 0, false);
	SetSolid(SOLID_VPHYSICS);
}

CON_COMMAND(rg_droppickup, "Creates an instance of the resource pickup.")
{
	EResourceType type = ResourceType_Biological;
	int worth = 1;
	if (args.ArgC() > 1)
	{
		const char* pArgType = args.Arg(1);

		int iType = Q_atoi(pArgType);

		if (iType < 1 || iType >= ResourceType_Count)
		{
			Warning("Invalid resource type. Must be a number between 1 and %i.", ResourceType_Count);
			return;
		}

		type = (EResourceType) iType;

		if (args.ArgC() > 2)
		{
			const char* pArgWorth = args.Arg(2);
			worth = Q_atoi(pArgWorth);

			if (worth <= 0)
			{
				Warning("Invalid worth amount, must be at least 1.");
				return;
			}
		}
	}

	Vector vecForward;
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if (!pPlayer)
	{
		Warning("Could not determine calling player!\n");
		return;
	}

	if (ResourceGathererRules())
	{
		AngleVectors(pPlayer->EyeAngles(), &vecForward);
		CResourceGathererResourcePickup *pEnt = ResourceGathererRules()->CreateResourcePickup(type, worth);
		if (pEnt)
		{
			Vector eyeOrigin = pPlayer->EyePosition();

			trace_t traceres;
			UTIL_TraceLine(origin, eyeOrigin + vecForward * 256, MASK_, nullptr, &traceres);
			QAngle vecAngles(0, pPlayer->GetAbsAngles().y - 90, 0);
			pEnt->SetAbsOrigin(traceres.endpos);
			pEnt->SetAbsAngles(vecAngles);

			DispatchSpawn(pEnt);

			Msg("Spawned resource \"%s\" with worth of %i.", ResourceTypeAsString(type), worth);
		}
	}
}