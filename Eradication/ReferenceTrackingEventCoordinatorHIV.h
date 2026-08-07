
#include "ReferenceTrackingEventCoordinator.h"

namespace Kernel
{
    struct IIndividualHumanHIV;
    struct IHIVMedicalHistory;

    ENUM_DEFINE(TargetDiseaseStateType,
        ENUM_VALUE_SPEC( Everyone                      , 1) 
        ENUM_VALUE_SPEC( HIV_Positive                  , 2) 
        ENUM_VALUE_SPEC( HIV_Negative                  , 3) 
        ENUM_VALUE_SPEC( Tested_Positive               , 4) 
        ENUM_VALUE_SPEC( Tested_Negative               , 5)
        ENUM_VALUE_SPEC( Not_Tested_Or_Tested_Negative , 6))

    class ReferenceTrackingEventCoordinatorHIV : public ReferenceTrackingEventCoordinator 
    {
        DECLARE_FACTORY_REGISTERED(EventCoordinatorFactory, ReferenceTrackingEventCoordinatorHIV, IEventCoordinator)    

    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        ReferenceTrackingEventCoordinatorHIV();
        virtual ~ReferenceTrackingEventCoordinatorHIV() { } 

        virtual bool Configure(const Configuration* config) override;
        virtual bool qualifiesDemographically( const IIndividualHumanEventContext* pIndividual );

    protected:
        IIndividualHumanHIV* GetIndividualHIV( const IIndividualHumanEventContext* pIndividual ) const;

        TargetDiseaseStateType::Enum target_disease_state;
    };
}
