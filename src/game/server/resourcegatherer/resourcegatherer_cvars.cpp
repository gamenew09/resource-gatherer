#include "cbase.h"
#include "resourcegatherer_cvars.h"

// Ready restart
ConVar rg_prototype_mode(
							"rg_prototype_mode", 
							"0", 
							FCVAR_GAMEDLL,
							"If non-zero, game will remove most of the features to be similar to the original Resource Collector prototype made by Valve." );