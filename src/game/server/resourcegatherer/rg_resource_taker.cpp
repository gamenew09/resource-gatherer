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

private:
    int m_iBiologicalCost;
    int m_iMechanicalCost;
    int m_iEnergyCost;

    bool m_bOneTime;
};

LINK_ENTITY_TO_CLASS( rg_resource_taker, CResourceGathererResourceTakerEntity );

// Start of our data description for the class
BEGIN_DATADESC( CMyLogicalEntity )
	DEFINE_KEYFIELD( m_iBiologicalCost, FIELD_INTEGER, "bio_cost" ),
	DEFINE_KEYFIELD( m_iMechanicalCost, FIELD_INTEGER, "mech_cost" ),
	DEFINE_KEYFIELD( m_iEnergyCost, FIELD_INTEGER, "energy_cost" ),

	DEFINE_KEYFIELD( m_bOneTime, FIELD_BOOLEAN, "one_time" ),

    DEFINE_INPUTFUNC( FIELD_VOID, "TakeResources", InputTakeResources ),

	DEFINE_OUTPUT( m_OnResourceTaken, "OnResourceTaken" ),
 
END_DATADESC()

void CResourceGathererResourceTakerEntity::InputTakeResources(inputdata_t& inputdata)
{
    CResourceGathererRules* pRules = ResourceGathererRules();
    if(pRules)
    {
        
        m_OnResourceTaken.FireOutput( pEnt, this );
    }
}