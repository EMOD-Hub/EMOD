
#include "stdafx.h"
#include "MalariaChallenge.h"

#include "Exceptions.h"
#include "NodeMalariaEventContext.h"  // for ISporozoiteChallengeConsumer methods

SETUP_LOGGING( "MalariaChallenge" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(MalariaChallenge)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(MalariaChallenge)

    IMPLEMENT_FACTORY_REGISTERED(MalariaChallenge)

    MalariaChallenge::MalariaChallenge()
        : BaseNodeIntervention()
        , challenge_type( MalariaChallengeType::InfectiousBites )
        , infectious_bite_count(1)
        , sporozoite_count(1)
        , coverage(1.0)
    {
        initSimTypes( 1, "MALARIA_SIM" );
    }

    MalariaChallenge::MalariaChallenge( const MalariaChallenge& master )
        : BaseNodeIntervention( master )
        , challenge_type( master.challenge_type )
        , infectious_bite_count( master.infectious_bite_count )
        , sporozoite_count( master.sporozoite_count )
        , coverage( master.coverage )
    {
    }

    bool MalariaChallenge::Configure( const Configuration * inputJson )
    {
        initConfig( "Challenge_Type", challenge_type, inputJson, MetadataDescriptor::Enum("Challenge_Type", MC_Challenge_Type_DESC_TEXT, MDD_ENUM_ARGS(MalariaChallengeType)) );
        initConfigTypeMap( "Coverage", &coverage, MC_Coverage_DESC_TEXT, 0, 1, 1 );

        // ----------------------------------------------------------------------------
        // --- I'm not setting Infectious_Bite_Count to depend on Challenge_Type,
        // --- it won't get read if Challenge_Type is absent. I did something similar
        // --- in SEC for Demographic_Coverage.
        // ----------------------------------------------------------------------------
        initConfigTypeMap( "Infectious_Bite_Count", &infectious_bite_count, MC_Infectious_Bite_Count_DESC_TEXT, 0, 1000, 1/*, "Challenge_Type", "InfectiousBites"*/ );
        initConfigTypeMap( "Sporozoite_Count", &sporozoite_count, MC_Sporozoite_Count_DESC_TEXT, 0, 1000, 1, "Challenge_Type", "Sporozoites" );

        bool is_configured = BaseNodeIntervention::Configure( inputJson );
        if( is_configured && !JsonConfigurable::_dryrun )
        {
            if( GET_CONFIG_STRING( EnvPtr->Config, "Malaria_Model" ) == "MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS" )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "'MalariaChallenge' cannot be used with parasite genetics.\nPlease use OutbreakIndividualMalariaGenetics." );
            }
        }
        return is_configured;
    }

    bool MalariaChallenge::Distribute(INodeEventContext *context, IEventCoordinator2* pEC)
    {
        return BaseNodeIntervention::Distribute( context, pEC );
    }

    void MalariaChallenge::Update( float dt )
    {
        if( AbortDueToDisqualifyingInterventionStatus( parent ) )
        {
            return;
        }

        ISporozoiteChallengeConsumer *iscc;
        if( s_OK != parent->QueryInterface( GET_IID( ISporozoiteChallengeConsumer ), (void**)&iscc ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "iscc", "ISporozoiteChallengeConsumer", "INodeEventContext" );
        }

        if( this->challenge_type == MalariaChallengeType::InfectiousBites )
        {
            iscc->ChallengeWithInfectiousBites( infectious_bite_count, coverage );
        }
        else if( this->challenge_type == MalariaChallengeType::Sporozoites )
        {
            iscc->ChallengeWithSporozoites( sporozoite_count, coverage );
        }
        else
        {
            release_assert( false );  // shouldn't get here
        }

        SetExpired( true );
    }
}
