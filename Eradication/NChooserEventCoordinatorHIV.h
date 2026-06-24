
#pragma once

#include "NChooserEventCoordinatorSTI.h"
#include "InterventionName.h"

namespace Kernel
{
    class TargetedDistributionHIV;
    struct IIndividualHumanSTI;
    struct IIndividualHumanHIV;
    struct IHIVMedicalHistory;

    ENUM_DEFINE(TargetedDiseaseState,
        ENUM_VALUE_SPEC( HIV_Positive               , 1) 
        ENUM_VALUE_SPEC( HIV_Negative               , 2) 
        ENUM_VALUE_SPEC( Tested_Positive            , 3) 
        ENUM_VALUE_SPEC( Tested_Negative            , 4)
        ENUM_VALUE_SPEC( Male_Circumcision_Positive , 5)
        ENUM_VALUE_SPEC( Male_Circumcision_Negative , 6)
        ENUM_VALUE_SPEC( Has_Intervention           , 7)
        ENUM_VALUE_SPEC( Not_Have_Intervention      , 8))

    // ------------------------------------------------------------------------
    // --- DiseaseQualificationsHIV
    // ------------------------------------------------------------------------

    // Extend DiseaseQualifications so we can check for the TargetedDiseaseState
    class DiseaseQualificationsHIV : public DiseaseQualifications
    {
    public:
        DiseaseQualificationsHIV( TargetedDistributionHIV* ptd );
        virtual ~DiseaseQualificationsHIV();

        // Return true of the person qualifies due to their specific disease state
        virtual bool Qualifies( IIndividualHumanEventContext *ihec ) const override;

    private:
        TargetedDistributionHIV* m_pTargetedDistribution;
    };

    // ------------------------------------------------------------------------
    // --- TargetedDistributionHIV
    // ------------------------------------------------------------------------

    class IDMAPI TargetedDistributionHIV : public TargetedDistributionSTI
    {
    public:
        TargetedDistributionHIV( NChooserObjectFactory* pObjectFactory );
        virtual ~TargetedDistributionHIV();

        virtual bool QualifiesByDiseaseState( IIndividualHumanEventContext *pHEC ) const;
    private:
        virtual void AddDiseaseConfiguration() override;
        virtual void CheckDiseaseConfiguration() override;

        static std::set< std::string > GetAllowedTargetDiseaseStates();
        static std::vector<std::vector<TargetedDiseaseState::Enum>> ConvertStringsToDiseaseState( std::vector<std::vector<std::string>>& rStringMatrix );

        virtual bool HasDiseaseState( TargetedDiseaseState::Enum state,
                                      const InterventionName& rHasInterventionName,
                                      IIndividualHumanEventContext *pHEC,
                                      IIndividualHumanSTI* pSTI,
                                      IIndividualHumanHIV *pHIV,
                                      IHIVMedicalHistory * pMedHistory ) const;

        std::set< std::string > m_AllowedStates; // member variable so that it exists shen JsonConfigurable::Configure() is called.
        std::vector<std::vector<std::string>> m_Vector2dStringDiseaseStates;
        std::vector<std::vector<TargetedDiseaseState::Enum>> m_DiseaseStates;
        std::string m_HasInterventionNameString;
        InterventionName m_HasInterventionName;
    };

    // ------------------------------------------------------------------------
    // --- NChooserObjectFactoryHIV
    // ------------------------------------------------------------------------
    // Extend so we can create TargetedDistributionHIV & DiseaseQualification objects
    class IDMAPI NChooserObjectFactoryHIV : public NChooserObjectFactorySTI
    {
    public:
        NChooserObjectFactoryHIV();
        virtual ~NChooserObjectFactoryHIV();

        virtual TargetedDistribution*  CreateTargetedDistribution() override;
        virtual DiseaseQualifications* CreateDiseaseQualifications( TargetedDistribution* ptd ) override;
    };

    // ------------------------------------------------------------------------
    // --- NChooserEventCoordinatorHIV
    // ------------------------------------------------------------------------

    class IDMAPI NChooserEventCoordinatorHIV : public NChooserEventCoordinatorSTI
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, NChooserEventCoordinatorHIV, IEventCoordinator)    
    public:
        NChooserEventCoordinatorHIV();
        NChooserEventCoordinatorHIV( NChooserObjectFactory* pObjectFactory );
        virtual ~NChooserEventCoordinatorHIV();

        virtual bool Configure( const Configuration * inputJson ) override;
    };
}
