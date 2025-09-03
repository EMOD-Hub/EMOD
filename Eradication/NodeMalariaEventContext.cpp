
#pragma once

#include "stdafx.h"
#include "NodeMalariaEventContext.h"
#include "InterventionsContainer.h" //IDrugVaccineInterventionEffects
#include "MalariaContexts.h" // for IMalariaHumanInfectable
#include "IInfectable.h"     // for IInfectionAcquirable
#include "RANDOM.h"
#include "StrainIdentity.h"
#include "IIndividualHuman.h"

SETUP_LOGGING( "NodeMalariaEventContext" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(NodeMalariaEventContextHost, NodeVectorEventContextHost)
        HANDLE_INTERFACE(ISporozoiteChallengeConsumer)
    END_QUERY_INTERFACE_DERIVED(NodeMalariaEventContextHost, NodeVectorEventContextHost)

    NodeMalariaEventContextHost::NodeMalariaEventContextHost(Node* _node) : NodeVectorEventContextHost(_node)
    { 
    }

    NodeMalariaEventContextHost::~NodeMalariaEventContextHost()
    {
    }

    void NodeMalariaEventContextHost::ChallengeWithSporozoites(int n_sporozoites, float coverage)
    {
        INodeEventContext::individual_visit_function_t sporozoite_challenge_func = 
            [this, n_sporozoites, coverage](IIndividualHumanEventContext *ihec)
        {
            IIndividualHumanVectorContext* p_ind_vector = nullptr;
            if(s_OK != ihec->QueryInterface(GET_IID(IIndividualHumanVectorContext), (void**)&p_ind_vector))
            {
                throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanVectorContext", "IIndividualHumanEventContext");
            }

            // Allow the bite challenge rate to be modified the individual factors that are usually handled in the Expose logic
            // GetAcquisitionImmunity() = Susceptibility::getModAcquire * InterventionsContainer::GetInterventionReducedAcquire
            // Susceptibility::getModAcquire is not used in Malaria, so remains 1.0
            // GetRelativeBitingRate() = demographics-based-risk::relative_biting_rate * m_age_dependent_biting_risk
            float relative_risk = p_ind_vector->GetRelativeBitingRate() * ihec->GetIndividualHumanConst()->GetAcquisitionImmunity();

            IMalariaHumanInfectable* imhi = nullptr;
            if ( s_OK !=  ihec->QueryInterface(GET_IID(IMalariaHumanInfectable), (void**)&imhi) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "ihec", "IMalariaHumanInfectable", "IIndividualHumanEventContext" );
            }

            if( GetRng()->SmartDraw( coverage*relative_risk ) && imhi->ChallengeWithSporozoites( n_sporozoites ) )
            {
                IInfectionAcquirable* iia = nullptr;
                if ( s_OK !=  ihec->QueryInterface(GET_IID(IInfectionAcquirable), (void**)&iia) )
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "ihec", "IInfectionAcquirable", "IIndividualHumanEventContext" );
                }
                StrainIdentity strain;
                iia->AcquireNewInfection( &strain );
            }
        };

        VisitIndividuals(sporozoite_challenge_func);
    }

    void NodeMalariaEventContextHost::ChallengeWithInfectiousBites(int n_bites, float coverage)
    {
        INodeEventContext::individual_visit_function_t infectious_bite_challenge_func = 
            [this, n_bites, coverage](IIndividualHumanEventContext *ihec)
        {
            IIndividualHumanVectorContext* p_ind_vector = nullptr;
            if(s_OK != ihec->QueryInterface(GET_IID(IIndividualHumanVectorContext), (void**)&p_ind_vector))
            {
                throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanVectorContext", "IIndividualHumanEventContext");
            }

            // Allow the bite challenge rate to be modified the individual factors that are usually handled in the Expose logic
            // GetAcquisitionImmunity() = Susceptibility::getModAcquire * InterventionsContainer::GetInterventionReducedAcquire
            // Susceptibility::getModAcquire is not used in Malaria, so remains 1.0
            // GetRelativeBitingRate() = demographics-based-risk::relative_biting_rate * m_age_dependent_biting_risk
            float relative_risk = p_ind_vector->GetRelativeBitingRate() * ihec->GetIndividualHumanConst()->GetAcquisitionImmunity();

            IMalariaHumanInfectable* imhi = nullptr;
            if ( s_OK !=  ihec->QueryInterface(GET_IID(IMalariaHumanInfectable), (void**)&imhi) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "ihec", "IMalariaHumanInfectable", "IIndividualHumanEventContext" );
            }

            if( GetRng()->SmartDraw( coverage*relative_risk ) && imhi->ChallengeWithBites( n_bites ) )
            {
                IInfectionAcquirable* iia = nullptr;
                if ( s_OK !=  ihec->QueryInterface(GET_IID(IInfectionAcquirable), (void**)&iia) )
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "ihec", "IInfectionAcquirable", "IIndividualHumanEventContext" );
                }
                StrainIdentity strain;
                iia->AcquireNewInfection( &strain );
            }
        };

        VisitIndividuals(infectious_bite_challenge_func);
    }
}