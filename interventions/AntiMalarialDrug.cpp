
#include "stdafx.h"
#include "AntiMalarialDrug.h"
#include "DrugModelAntiMalarial.h"

#include "Exceptions.h"
#include "IIndividualHumanContext.h"
#include "IndividualEventContext.h"
#include "MalariaDrugTypeParameters.h"
#include "MalariaContexts.h"

SETUP_LOGGING( "AntimalarialDrug" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(AntimalarialDrug, GenericDrug)
        HANDLE_INTERFACE(IMalariaDrugEffects)
        HANDLE_INTERFACE( IReportInterventionDataAccess )
    END_QUERY_INTERFACE_DERIVED(AntimalarialDrug, GenericDrug)
    IMPLEMENT_FACTORY_REGISTERED(AntimalarialDrug)

    AntimalarialDrug::AntimalarialDrug()
        : GenericDrug( JsonConfigurable::default_string, new DrugModelAntiMalarial() )
        , imda( nullptr )
    {
        initSimTypes( 1, "MALARIA_SIM" );
    }

    AntimalarialDrug::AntimalarialDrug( const AntimalarialDrug& rThat )
        : GenericDrug( rThat )
        , tmp_drug_name()
        , imda( rThat.imda )
    {
    }

    AntimalarialDrug::~AntimalarialDrug()
    {
    }

    float
    AntimalarialDrug::get_drug_IRBC_killrate( const IStrainIdentity& rStrain )
    {
        return static_cast<DrugModelAntiMalarial*>(p_drug_model)->get_drug_IRBC_killrate( rStrain );
    }

    float AntimalarialDrug::get_drug_hepatocyte( const IStrainIdentity& rStrain )
    {
        return static_cast<DrugModelAntiMalarial*>(p_drug_model)->get_drug_hepatocyte( rStrain );
    }

    float
    AntimalarialDrug::get_drug_gametocyte02( const IStrainIdentity& rStrain )
    {
        return static_cast<DrugModelAntiMalarial*>(p_drug_model)->get_drug_gametocyte02( rStrain );
    }

    float
    AntimalarialDrug::get_drug_gametocyte34( const IStrainIdentity& rStrain )
    {
        return static_cast<DrugModelAntiMalarial*>(p_drug_model)->get_drug_gametocyte34( rStrain );
    }

    float
    AntimalarialDrug::get_drug_gametocyteM( const IStrainIdentity& rStrain )
    {
        return static_cast<DrugModelAntiMalarial*>(p_drug_model)->get_drug_gametocyteM( rStrain );
    }

    void AntimalarialDrug::ConfigureDrugType( const Configuration *intputJson )
    {
        tmp_drug_name.constraints = "Malaria_Drug_Params[X].Name";
        tmp_drug_name.constraint_param = nullptr;
        if( !JsonConfigurable::_dryrun )
        {
            tmp_drug_name.constraint_param = &(MalariaDrugTypeCollection::GetInstance()->GetDrugNames() );
        }

        initConfigTypeMap( "Drug_Type", &tmp_drug_name, Drug_Type_DESC_TEXT );
    }

    void AntimalarialDrug::CheckConfigureDrugType( const Configuration *inputJson )
    {
        if( tmp_drug_name.empty() || !inputJson->Exist( "Drug_Type" ) )
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "'Drug_Type' was not defined and it is a required parameter." );
        }

        const MalariaDrugTypeParameters* p_params = &(MalariaDrugTypeCollection::GetInstance()->GetDrug( tmp_drug_name ));

        delete p_drug_model;
        p_drug_model = new DrugModelAntiMalarial( p_params->GetName(), p_params );
    }

    bool AntimalarialDrug::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0, 999999, 10);
        ConfigureDrugType( inputJson );

        bool ret = BaseIntervention::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            CheckConfigureDrugType( inputJson );
        }
        return ret;
    }

    bool
    AntimalarialDrug::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        if (s_OK != context->QueryInterface(GET_IID(IMalariaDrugEffectsApply), (void**)&imda) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IMalariaDrugEffectsApply", "IIndividualHumanInterventionsContext" );
        }

        // just add in another Drug to list, can later check the person's records and apply accordingly (TODO)
        return GenericDrug::Distribute( context, pCCO );
    }

    void
    AntimalarialDrug::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        release_assert( context );
        release_assert( context->GetInterventionsContext() );
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IMalariaDrugEffectsApply), (void**)&imda) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context->GetInterventionsContext()", "IMalariaDrugEffectsApply", "IIndividualHumanInterventionsContext" );
        }

        return GenericDrug::SetContextTo( context );
    }

    int AntimalarialDrug::GetNumDoses() const
    {
        release_assert( p_drug_model != nullptr );
        DrugModelAntiMalarial* p_drug_model_malaria = static_cast<DrugModelAntiMalarial*>(p_drug_model);
        int num_doses = p_drug_model_malaria->GetDrugParams()->GetFullTreatmentDoses();
        return num_doses;
    }

    float AntimalarialDrug::GetDoseInterval() const
    {
        release_assert( p_drug_model != nullptr );
        DrugModelAntiMalarial* p_drug_model_malaria = static_cast<DrugModelAntiMalarial*>(p_drug_model);
        return p_drug_model_malaria->GetDrugParams()->GetDoseInterval();
    }


    void AntimalarialDrug::ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc )
    {
        remaining_doses = GetNumDoses();
        time_between_doses = GetDoseInterval();

        GenericDrug::ConfigureDrugTreatment( ivc );
    }

    void AntimalarialDrug::ResetForNextDose(float dt)
    {
        dosing_timer = time_between_doses;
        remaining_doses--;

        if (remaining_doses > 0 && time_between_doses < dt)
        {
            LOG_DEBUG_F("Time to next dose (%0.3f) is shorter than the time-step (%0.3f).\n", time_between_doses, dt);

            // Decrement remaining_doses further.  For example, if we are doing 1h timesteps for infected individuals but 24h for uninfected
            // and have 12h dosing (6 pills over 3 days), do 3 "double" pills over 3 days for 24h updaters instead of what otherwise would be 6 pills over 6 days
            int decrement_n_more_doses = int(dt/time_between_doses + 0.5)-1;
            remaining_doses -= min(remaining_doses, decrement_n_more_doses);
            LOG_DEBUG_F("Decrementing remaining doses by another %d\n", decrement_n_more_doses);
        }
    }

    void AntimalarialDrug::ApplyEffects()
    {
        assert(imda);

        imda->AddDrugEffects( this );
    }

    void AntimalarialDrug::Expire()
    {
        imda->RemoveDrugEffects( this );
        GenericDrug::Expire();
    }

    ReportInterventionData AntimalarialDrug::GetReportInterventionData() const
    {
        ReportInterventionData data = BaseIntervention::GetReportInterventionData();

        data.concentration_drug = GetDrugCurrentConcentration();

        return data;
    }

    REGISTER_SERIALIZABLE(AntimalarialDrug);

    void AntimalarialDrug::serialize(IArchive& ar, AntimalarialDrug* obj)
    {
        GenericDrug::serialize(ar, obj);
        AntimalarialDrug& drug = *obj;

        // imda updated in SetContextTo()
    }
}
