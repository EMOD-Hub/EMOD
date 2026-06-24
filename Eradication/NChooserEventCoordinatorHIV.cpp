
#include "stdafx.h"

#include "NChooserEventCoordinatorHIV.h"
#include "IIndividualHumanSTI.h"
#include "IIndividualHumanHIV.h"
#include "IHIVInterventionsContainer.h"
#include "SimulationConfig.h"

SETUP_LOGGING( "NChooserEventCoordinatorHIV" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- DiseaseQualificationsHIV
    // ------------------------------------------------------------------------

    DiseaseQualificationsHIV::DiseaseQualificationsHIV( TargetedDistributionHIV* ptd )
    : DiseaseQualifications()
    , m_pTargetedDistribution( ptd )
    {
    }

    DiseaseQualificationsHIV::~DiseaseQualificationsHIV()
    {
    }

    bool DiseaseQualificationsHIV::Qualifies( IIndividualHumanEventContext *ihec ) const
    {
        return m_pTargetedDistribution->QualifiesByDiseaseState( ihec );
    }

    // ------------------------------------------------------------------------
    // --- TargetedDistributionHIV
    // ------------------------------------------------------------------------

    TargetedDistributionHIV::TargetedDistributionHIV( NChooserObjectFactory* pObjectFactory )
    : TargetedDistributionSTI(pObjectFactory)
    , m_Vector2dStringDiseaseStates()
    , m_DiseaseStates()
    , m_HasInterventionNameString()
    , m_HasInterventionName()
    {
    }

    TargetedDistributionHIV::~TargetedDistributionHIV()
    {
    }

    void TargetedDistributionHIV::AddDiseaseConfiguration()
    {
        m_AllowedStates = TargetedDistributionHIV::GetAllowedTargetDiseaseStates();

        initConfigTypeMap("Target_Disease_State",                       &m_Vector2dStringDiseaseStates, NC_HIV_TD_Target_Disease_State_DESC_TEXT, nullptr, m_AllowedStates );
        initConfigTypeMap("Target_Disease_State_Has_Intervention_Name", &m_HasInterventionNameString,   NC_HIV_TD_Has_Intervention_Name_DESC_TEXT, "" );
     }

    void TargetedDistributionHIV::CheckDiseaseConfiguration()
    {
        if( !m_HasInterventionNameString.empty() )
        {
            m_HasInterventionName = m_HasInterventionNameString;
        }
        m_DiseaseStates = TargetedDistributionHIV::ConvertStringsToDiseaseState( m_Vector2dStringDiseaseStates );

        for( auto& inner : m_DiseaseStates )
        {
            for( auto state : inner )
            {
                if( ( (state == TargetedDiseaseState::Has_Intervention     ) || 
                      (state == TargetedDiseaseState::Not_Have_Intervention)  ) &&
                    m_HasInterventionName.empty() )
                {
                    const char* state_name = TargetedDiseaseState::pairs::lookup_key( state );
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                        "Target_Disease_State", state_name,
                        "Target_Disease_State_Has_Intervention_Name", "<empty>", 
                        "If using 'Has_Intervention' or 'Not_Have_Intervention', you must also define 'Target_Disease_State_Has_Intervention_Name' to the name of the intervention." );
                }
            }
        }
        m_Vector2dStringDiseaseStates.clear();
        m_AllowedStates.clear();
    }

    std::set< std::string > TargetedDistributionHIV::GetAllowedTargetDiseaseStates()
    {
        std::set< std::string > allowed;
        for( int i = 0 ; i < TargetedDiseaseState::pairs::count() ; ++i )
        {
            allowed.insert( std::string(TargetedDiseaseState::pairs::get_keys()[i]) );
        }

        return allowed;
    }

    std::vector<std::vector<TargetedDiseaseState::Enum>> TargetedDistributionHIV::ConvertStringsToDiseaseState( std::vector<std::vector<std::string>>& rStringMatrix )
    {
        std::vector<std::vector<TargetedDiseaseState::Enum>> enum_matrix;
        for( auto& vec : rStringMatrix )
        {
            std::vector<TargetedDiseaseState::Enum> enum_vec ;
            for( auto& str : vec )
            {
                 int int_state = TargetedDiseaseState::pairs::lookup_value( str.c_str() );
                 if( int_state == -1 )
                 {
                    std::stringstream ss ;
                    ss << "The 'Target_Disease_State' value of '" << str << "' is not a valid enum of TargetedDiseaseState.  Valid values are:\n";
                    for( int i = 0 ; i < TargetedDiseaseState::pairs::count() ; ++i )
                    {
                        ss << TargetedDiseaseState::pairs::get_keys()[i] << "\n" ;
                    }
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                 }
                 TargetedDiseaseState::Enum state = (TargetedDiseaseState::Enum)int_state;
                 enum_vec.push_back( state );
            }
            enum_matrix.push_back( enum_vec );
        }
        return enum_matrix;
    }

    bool TargetedDistributionHIV::HasDiseaseState( TargetedDiseaseState::Enum state,
                                                   const InterventionName& rHasInterventionName,
                                                   IIndividualHumanEventContext *pHEC,
                                                   IIndividualHumanSTI* pSTI,
                                                   IIndividualHumanHIV *pHIV,
                                                   IHIVMedicalHistory * pMedHistory ) const
    {
        switch( state )
        {
            case TargetedDiseaseState::HIV_Positive:
                return pHIV->HasHIV();

            case TargetedDiseaseState::HIV_Negative:
                return !pHIV->HasHIV();

            case TargetedDiseaseState::Tested_Positive:
                return pMedHistory->EverTestedHIVPositive();

            case TargetedDiseaseState::Tested_Negative:
                return (pMedHistory->EverTested() && !pMedHistory->EverTestedHIVPositive());

            case TargetedDiseaseState::Male_Circumcision_Positive:
                return pSTI->IsCircumcised();

            case TargetedDiseaseState::Male_Circumcision_Negative:
                return !pSTI->IsCircumcised();

            case TargetedDiseaseState::Has_Intervention:
                return pHEC->GetInterventionsContext()->ContainsExistingByName( rHasInterventionName );

            case TargetedDiseaseState::Not_Have_Intervention:
                return !(pHEC->GetInterventionsContext()->ContainsExistingByName( rHasInterventionName ));
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "TargetedDiseaseState", state, TargetedDiseaseState::pairs::lookup_key( state ) );
        }
    }

    bool TargetedDistributionHIV::QualifiesByDiseaseState( IIndividualHumanEventContext *pHEC ) const
    {
        if( m_DiseaseStates.size() == 0 )
        {
            return true;
        }

        IIndividualHumanHIV * p_ind_hiv = nullptr;
        if( pHEC->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&p_ind_hiv) != s_OK )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "ihec", "IIndividualHumanHIV", "IIndividualHumanEventContext");
        }
        IIndividualHumanSTI* p_ind_sti = p_ind_hiv->GetIndividualHumanSTI();;

        IHIVMedicalHistory * p_med_history = p_ind_hiv->GetMedicalHistory();

        for( auto& states_to_and : m_DiseaseStates )
        {
            bool qualifies = true;
            for( int i = 0 ; qualifies && (i < states_to_and.size()) ; ++i )
            {
                auto state = states_to_and[i];
                qualifies = HasDiseaseState( state, m_HasInterventionName, pHEC, p_ind_sti, p_ind_hiv, p_med_history );
            }
            if( qualifies )
            {
                return true;
            }
        }
        return false;
    }


    // ------------------------------------------------------------------------
    // --- NChooserObjectFactoryHIV
    // ------------------------------------------------------------------------

    NChooserObjectFactoryHIV::NChooserObjectFactoryHIV()
    {
    }

    NChooserObjectFactoryHIV::~NChooserObjectFactoryHIV()
    {
    }

    TargetedDistribution* NChooserObjectFactoryHIV::CreateTargetedDistribution()
    {
        return new TargetedDistributionHIV( this );
    }

    DiseaseQualifications* NChooserObjectFactoryHIV::CreateDiseaseQualifications( TargetedDistribution* ptd )
    {
        TargetedDistributionHIV* p_hiv_td = static_cast<TargetedDistributionHIV*>(ptd);
        return new DiseaseQualificationsHIV( p_hiv_td );
    }

    // ------------------------------------------------------------------------
    // --- NChooserEventCoordinatorHIV
    // ------------------------------------------------------------------------

    IMPLEMENT_FACTORY_REGISTERED(NChooserEventCoordinatorHIV)

    NChooserEventCoordinatorHIV::NChooserEventCoordinatorHIV()
    : NChooserEventCoordinatorSTI( new NChooserObjectFactoryHIV() )
    {
    }

    NChooserEventCoordinatorHIV::NChooserEventCoordinatorHIV( NChooserObjectFactory* pObjectFactory )
    : NChooserEventCoordinatorSTI( pObjectFactory )
    {
    }

    NChooserEventCoordinatorHIV::~NChooserEventCoordinatorHIV()
    {
    }

    bool NChooserEventCoordinatorHIV::Configure( const Configuration * inputJson )
    {
        if( !JsonConfigurable::_dryrun &&
            (GET_CONFIGURABLE( SimulationConfig )->sim_type != SimType::HIV_SIM) )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "NChooserEventCoordinatorHIV can only be used in HIV simulations." );
        }
        return NChooserEventCoordinatorSTI::Configure( inputJson );
    }
}

