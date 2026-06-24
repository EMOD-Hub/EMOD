
#include "stdafx.h"

#include "ReferenceTrackingEventCoordinatorHIV.h"
#include "IIndividualHumanHIV.h"
#include "IHIVInterventionsContainer.h"

SETUP_LOGGING( "ReferenceTrackingEventCoordinatorHIV" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(ReferenceTrackingEventCoordinatorHIV)
    BEGIN_QUERY_INTERFACE_BODY(ReferenceTrackingEventCoordinatorHIV)
    END_QUERY_INTERFACE_BODY(ReferenceTrackingEventCoordinatorHIV)

    ReferenceTrackingEventCoordinatorHIV::ReferenceTrackingEventCoordinatorHIV()
    : ReferenceTrackingEventCoordinator()
    , target_disease_state(TargetDiseaseStateType::Everyone)
    {
    }

    bool
    ReferenceTrackingEventCoordinatorHIV::Configure(
        const Configuration * inputJson
    )
    {
        initConfig("Target_Disease_State", target_disease_state, inputJson, MetadataDescriptor::Enum("Target_Disease_State", RTEC_HIV_Target_Disease_State_DESC_TEXT, MDD_ENUM_ARGS(TargetDiseaseStateType))) ;

        auto ret = ReferenceTrackingEventCoordinator::Configure( inputJson );
        return ret;
    }

    IIndividualHumanHIV* ReferenceTrackingEventCoordinatorHIV::GetIndividualHIV( const IIndividualHumanEventContext * pIndividual ) const
    {
        IIndividualHumanHIV* p_hiv_individual = nullptr;
        if(const_cast<IIndividualHumanEventContext*>(pIndividual)->QueryInterface( GET_IID( IIndividualHumanHIV ), (void**)&p_hiv_individual ) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                            "pIndividual",
                                            "IIndividualHumanHIV",
                                            "IIndividualHumanEventContext" );
        }
        return p_hiv_individual;
    }

    bool ReferenceTrackingEventCoordinatorHIV::qualifiesDemographically( const IIndividualHumanEventContext * pIndividual )
    {
        bool qualifies = ReferenceTrackingEventCoordinator::qualifiesDemographically( pIndividual );
        if( qualifies )
        {
            switch( target_disease_state )
            {
                case TargetDiseaseStateType::Everyone:
                    qualifies = true;
                    break;
                case TargetDiseaseStateType::HIV_Positive:
                    qualifies = GetIndividualHIV( pIndividual )->HasHIV();
                    break;
                case TargetDiseaseStateType::HIV_Negative:
                    qualifies = !GetIndividualHIV( pIndividual )->HasHIV();
                    break;
                case TargetDiseaseStateType::Tested_Positive:
                    qualifies = GetIndividualHIV( pIndividual )->GetMedicalHistory()->EverTestedHIVPositive();
                    break;
                case TargetDiseaseStateType::Tested_Negative:
                {
                    IHIVMedicalHistory* p_med_history = GetIndividualHIV( pIndividual )->GetMedicalHistory();
                    qualifies = p_med_history->EverTested() && !p_med_history->EverTestedHIVPositive();
                    break;
                }
                case TargetDiseaseStateType::Not_Tested_Or_Tested_Negative:
                {
                    IHIVMedicalHistory* p_med_history = GetIndividualHIV( pIndividual )->GetMedicalHistory();
                    qualifies = !p_med_history->EverTested() || !p_med_history->EverTestedHIVPositive();
                    break;
                }
                default:
                    throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "target_disease_state", target_disease_state, TargetDiseaseStateType::pairs::lookup_key( target_disease_state ) );
            }
        }
        return qualifies;
    }
}

