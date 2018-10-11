
#include "cbase.h"

#define ACHIEVEMENT_RG_COLLECT_THOUSAND_BIOLOGICAL 1

#if defined(RESOURCEGATHERER_CLIENT_DLL) && defined(RESOURCEGATHERER_ENABLE_ACHIEVEMENTS)

#include "achievementmgr.h"
#include "baseachievement.h"

// Get's the local player on the client-side.
// This macro is defined for making code writing a bit faster.
#define UTIL_GetLocalPlayer() C_BasePlayer::GetLocalPlayer()

#include "resourcegatherer_gamerules.h"

CAchievementMgr g_AchievementMgrResourceGatherer; // Resource Gatherer achievement manager.

class CAchievementRGCollectThousandBiological : public CBaseAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_MAP_EVENTS | ACH_SAVE_GLOBAL);
		SetGoal( 1000 );
    }

    virtual void ListenForEvents()
	{
		ListenForGameEvent( "resource_added" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( 0 == Q_strcmp( event->GetName(), "resource_added" ) )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByUserId( event->GetInt( "causer", 0 ) );
			if ( pPlayer == UTIL_GetLocalPlayer() && event->GetInt("resource_type") == ResourceType_Biological) // Make sure we only count biological resources collected by the local player.
			{
				IncrementCount(event->GetInt("amount"));
			}
		}
    }
};
DECLARE_ACHIEVEMENT(CAchievementRGCollectThousandBiological, ACHIEVEMENT_RG_COLLECT_THOUSAND_BIOLOGICAL, "ACHIEVEMENT_RG_COLLECT_THOUSAND_BIOLOGICAL", 10);

#define DECLARE_RG_MAP_EVENT_ACHIEVEMENT( achievementID, achievementName, iPointValue ) \
	DECLARE_MAP_EVENT_ACHIEVEMENT_(achievementID, achievementName, "resourcegatherer", iPointValue, false)



#endif