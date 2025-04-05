
#include "stdafx.h"
#include "MultiInterventionDistributor.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanContext.h"
#include "ISimulationContext.h"

SETUP_LOGGING( "MultiInterventionDistributor" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(MultiInterventionDistributor)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(MultiInterventionDistributor)

    IMPLEMENT_FACTORY_REGISTERED(MultiInterventionDistributor)

    bool MultiInterventionDistributor::Configure( const Configuration * inputJson )
    {
        IndividualInterventionConfig intervention_list;
        initConfigComplexType("Intervention_List", &intervention_list, MID_Intervention_List_DESC_TEXT);

        bool ret = BaseIntervention::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            InterventionFactory::CreateInterventionList( intervention_list._json, inputJson->GetDataLocation(), "Intervention_List", m_Interventions );
        }
        return ret ;
    }

    MultiInterventionDistributor::MultiInterventionDistributor()
    : BaseIntervention()
    , m_Interventions()
    {
    }

    MultiInterventionDistributor::MultiInterventionDistributor( const MultiInterventionDistributor& rMaster )
    : BaseIntervention( rMaster )
    , m_Interventions()
    {
        for( auto p_master_intervention : rMaster.m_Interventions )
        {
            m_Interventions.push_back( p_master_intervention->Clone() );
        }
    }

    MultiInterventionDistributor::~MultiInterventionDistributor()
    {
        for( auto p_intervention : m_Interventions )
        {
            delete p_intervention;
        }
    }

    void MultiInterventionDistributor::Update( float dt )
    {
        // nothing to do here.  this intervention just distributes its list of interventions and then expires
    }
    
    bool MultiInterventionDistributor::Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO )
    {
        // ----------------------------------------------------------------------------------
        // --- Putting this here because we don't want anything to happen if we are aborting
        // ----------------------------------------------------------------------------------
        if( AbortDueToDisqualifyingInterventionStatus( context->GetParent() ) )
        {
            return false;
        }

        for( auto p_intervention : m_Interventions )
        {
            IDistributableIntervention* p_di = p_intervention->Clone();
            p_di->AddRef();
            p_di->Distribute( context, pICCO );
            p_di->Release();
        }

        // Nothing more for this class to do...
        Expire();

        return true;
    }

    void MultiInterventionDistributor::Expire()
    {
        expired = true;
    }
}
