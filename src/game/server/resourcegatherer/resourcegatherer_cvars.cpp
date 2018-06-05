#include "cbase.h"
#include "resourcegatherer_cvars.h"

// Prototype Mode (Currently unused)
ConVar rg_prototype_mode(
							"rg_prototype_mode", 
							"0", 
							FCVAR_GAMEDLL,
							"If non-zero, game will remove most of the features to be similar to the original Resource Collector prototype made by Valve." );

//sk_resource_drop_barnacle
ConVar sk_resource_drop_per_damage_barnacle(
	"sk_resource_drop_per_damage_barnacle",
	"2",
	FCVAR_GAMEDLL,
	"A skill variable that determines how many bio resources will be dropped per damage.");
