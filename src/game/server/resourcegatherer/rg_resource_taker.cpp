#include "cbase.h"
#include "resourcegatherer_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CResourceGathererResourceTakerEntity : public CLogicalEntity
{
public:
	DECLARE_CLASS( CResourceGathererResourceTakerEntity, CLogicalEntity );
    DECLARE_DATADESC();

    CResourceGathererResourceTakerEntity()
    {
        m_iBiologicalCost = m_iMechanicalCost = m_iEnergyCost = 0;
        m_bOneTime = true;
    }

    void InputTakeResources(inputdata_t& inputdata);
	void InputResetActivationState(inputdata_t& inputdata);

	bool HasActivated() const { return m_bHasActivated; }

	void ResetActivatedState() { m_bHasActivated = false; }

private:
    int m_iBiologicalCost;
    int m_iMechanicalCost;
    int m_iEnergyCost;

	bool m_bHasActivated;

    bool m_bOneTime;

	COutputEvent m_OnResourceTaken;
	COutputEvent m_OnResourcesTakenFailed;
	COutputEvent m_OnAttemptTakeButAlreadyTaken;
};

LINK_ENTITY_TO_CLASS( rg_resource_taker, CResourceGathererResourceTakerEntity );

// Start of our data description for the class
BEGIN_DATADESC(CResourceGathererResourceTakerEntity)
	DEFINE_KEYFIELD( m_iBiologicalCost, FIELD_INTEGER, "bio_cost" ),
	DEFINE_KEYFIELD( m_iMechanicalCost, FIELD_INTEGER, "mech_cost" ),
	DEFINE_KEYFIELD( m_iEnergyCost, FIELD_INTEGER, "energy_cost" ),

	DEFINE_KEYFIELD( m_bOneTime, FIELD_BOOLEAN, "one_time" ),

    DEFINE_INPUTFUNC( FIELD_VOID, "TakeResources", InputTakeResources ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ResetActivationState", InputResetActivationState ),

	DEFINE_OUTPUT( m_OnResourceTaken, "OnResourcesTaken" ),
	DEFINE_OUTPUT( m_OnResourcesTakenFailed, "OnResourcesTakenFailed"),
	DEFINE_OUTPUT( m_OnAttemptTakeButAlreadyTaken, "OnAttemptTakeButAlreadyTaken" ),
 
END_DATADESC()

void CResourceGathererResourceTakerEntity::InputResetActivationState(inputdata_t& inputdata)
{
	m_bHasActivated = false;
}

void CResourceGathererResourceTakerEntity::InputTakeResources(inputdata_t& inputdata)
{
	if (m_bOneTime && m_bHasActivated)
	{
		m_OnAttemptTakeButAlreadyTaken.FireOutput(inputdata.pActivator, this);
		return;
	}

    CResourceGathererRules* pRules = ResourceGathererRules();
	CBasePlayer* pPlayer = dynamic_cast<CBasePlayer*>(inputdata.pActivator);

    if(pRules)
    {
		bool bTakeResources = true;

		if (m_iBiologicalCost >= 0)
		{
			if (pRules->GetBiologicalResourceCount() < m_iBiologicalCost)
			{
				bTakeResources = false;
			}
		}

		if (m_iMechanicalCost >= 0)
		{
			if (pRules->GetMechanicalResourceCount() < m_iMechanicalCost)
			{
				bTakeResources = false;
			}
		}

		if (m_iEnergyCost >= 0)
		{
			if (pRules->GetEnergyResourceCount() < m_iEnergyCost)
			{
				bTakeResources = false;
			}
		}

		if (bTakeResources)
		{
			if (m_iBiologicalCost >= 0)
			{
				pRules->TakeResource(pPlayer, ResourceType_Biological, m_iBiologicalCost);
			}

			if (m_iMechanicalCost >= 0)
			{
				pRules->TakeResource(pPlayer, ResourceType_Mechanical, m_iMechanicalCost);
			}

			if (m_iEnergyCost >= 0)
			{
				pRules->TakeResource(pPlayer, ResourceType_Energy, m_iEnergyCost);
			}

			m_OnResourceTaken.FireOutput(inputdata.pActivator, this);
			m_bHasActivated = true;
		}
		else
		{
			m_OnResourcesTakenFailed.FireOutput(inputdata.pActivator, this);
		}
    }
}