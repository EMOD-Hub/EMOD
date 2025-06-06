
#include "stdafx.h"
#include "MultiNodeInterventionDistributor.h"
#include "NodeEventContext.h"
#include "ISimulationContext.h"

SETUP_LOGGING( "MultiNodeInterventionDistributor" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( MultiNodeInterventionDistributor )
        HANDLE_INTERFACE( IConfigurable )
        HANDLE_INTERFACE( IBaseIntervention )
        HANDLE_INTERFACE( INodeDistributableIntervention )
        HANDLE_ISUPPORTS_VIA( INodeDistributableIntervention )
    END_QUERY_INTERFACE_BODY( MultiNodeInterventionDistributor )

    IMPLEMENT_FACTORY_REGISTERED( MultiNodeInterventionDistributor )

    bool MultiNodeInterventionDistributor::Configure( const Configuration * inputJson )
    {
        NodeInterventionConfig intervention_list;
        initConfigComplexType( "Node_Intervention_List", &intervention_list, MNID_Node_Intervention_List_DESC_TEXT );

        bool ret = BaseNodeIntervention::Configure( inputJson );
        if( ret & !JsonConfigurable::_dryrun )
        {
            InterventionFactory::CreateNDIInterventionList( intervention_list._json, inputJson->GetDataLocation(), "Node_Intervention_List", m_Interventions );
        }
        return ret;
    }

    MultiNodeInterventionDistributor::MultiNodeInterventionDistributor()
    : BaseNodeIntervention()
    , m_Interventions()
    {
    }

    MultiNodeInterventionDistributor::MultiNodeInterventionDistributor( const MultiNodeInterventionDistributor& rMaster )
    : BaseNodeIntervention( rMaster )
    , m_Interventions()
    {
        for( auto p_master_intervention : rMaster.m_Interventions )
        {
            m_Interventions.push_back( p_master_intervention->Clone() );
        }
    }

    MultiNodeInterventionDistributor::~MultiNodeInterventionDistributor()
    {
        for( auto p_intervention : m_Interventions )
        {
            delete p_intervention;
        }
    }

    void MultiNodeInterventionDistributor::Update( float dt )
    {
        // nothing to do here.  this intervention just distributes its list of interventions and then expires
        release_assert( false );
    }

    bool MultiNodeInterventionDistributor::Distribute( INodeEventContext *context, IEventCoordinator2* pEC )
    {
        // ----------------------------------------------------------------------------------
        // --- Putting this here because we don't want anything to happen if we are aborting
        // ----------------------------------------------------------------------------------
        if( AbortDueToDisqualifyingInterventionStatus( context ) )
        {
            return false;
        }

        for( auto p_intervention : m_Interventions )
        {
            INodeDistributableIntervention* p_ndi = p_intervention->Clone();
            p_ndi->AddRef();
            p_ndi->Distribute( context, pEC );
            p_ndi->Release();
        }

        // Nothing more for this class to do...
        SetExpired( true );

        return true;
    }
}
