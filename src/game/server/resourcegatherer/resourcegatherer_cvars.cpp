#include "cbase.h"
#include "resourcegatherer_cvars.h"

// Prototype Mode (Currently unused)
ConVar rg_prototype_mode(
							"rg_prototype_mode", 
							"0", 
							FCVAR_GAMEDLL | FCVAR_NOTIFY,
							"If non-zero, game will remove most of the features to be similar to the original Resource Collector prototype made by Valve." );

ConVar rg_resource_pickup_radius(
	"rg_resource_pickup_radius",
	"25",
	FCVAR_GAMEDLL | FCVAR_NOTIFY,
	"How close does a player have to be in order to pickup the resource.");

//sk_resource_drop_barnacle
ConVar sk_resource_drop_per_damage_barnacle(
	"sk_resource_drop_per_damage_barnacle",
	"2",
	FCVAR_GAMEDLL | FCVAR_NOTIFY,
	"A skill variable that determines how many bio resources will be dropped per damage for a Barnacle.");

//sk_resource_drop_per_damage_combine_soldier
ConVar sk_resource_drop_per_damage_combine_soldier(
	"sk_resource_drop_per_damage_combine_soldier",
	"4",
	FCVAR_GAMEDLL | FCVAR_NOTIFY,
	"A skill variable that determines how many bio resources will be dropped per damage for a Combine Soldier.");

ConVar rg_restartmap(
	"rg_restartmap",
	"1",
	FCVAR_GAMEDLL | FCVAR_NOTIFY,
	"If non-zero, the game will restart the map instead of changing maps once the game is over.");