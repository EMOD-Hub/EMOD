
#include "stdafx.h"
#include "Drugs.h"
#include "RANDOM.h"
#include "Sigmoid.h"
#include "SimulationEnums.h"        // Just for PKPDModel parameter (!)
#include "IIndividualHumanContext.h"

SETUP_LOGGING( "GenericDrug" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(GenericDrug)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(IDrug)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(GenericDrug)

#ifndef INTERVENTIONS_AS_DLLS
    //IMPLEMENT_FACTORY_REGISTERED(GenericDrug)
#endif
    bool
    GenericDrug::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap("Cost_To_Consumer",            &cost_per_unit,                          IV_Cost_To_Consumer_DESC_TEXT,               0, 99999);
        initConfigTypeMap("Remaining_Doses",             &remaining_doses,                        GD_Remaining_Doses_DESC_TEXT,               -1, 999999);
        initConfigTypeMap("Dose_Interval",               &time_between_doses,                     GD_Dose_Interval_DESC_TEXT,               0.0f, 99999.0f, 1.0f);
        initConfigTypeMap("Primary_Decay_Time_Constant", &p_drug_model->fast_decay_time_constant, GD_Primary_Decay_Time_Constant_DESC_TEXT,    0, 999999);
        initConfigTypeMap("Fraction_Defaulters",         &p_drug_model->fraction_defaulters,      GD_Fraction_Defaulters_DESC_TEXT,         0.0f, 1.0f, 0.0f);

        initConfig( "Durability_Profile", p_drug_model->durability_time_profile, inputJson, MetadataDescriptor::Enum("Durability_Profile", GD_Durability_Profile_DESC_TEXT, MDD_ENUM_ARGS(PKPDModel)) );
        if (p_drug_model->durability_time_profile == PKPDModel::CONCENTRATION_VERSUS_TIME || JsonConfigurable::_dryrun)
        {
            initConfigTypeMap("Secondary_Decay_Time_Constant", &p_drug_model->slow_decay_time_constant, GD_Secondary_Decay_Time_Constant_DESC_TEXT, 0, 999999);
            initConfigTypeMap("Drug_CMax",                     &p_drug_model->Cmax,                     GD_Drug_CMax_DESC_TEXT,                     0, 10000);
            initConfigTypeMap("Drug_Vd",                       &p_drug_model->Vd,                       GD_Drug_Vd_DESC_TEXT,                       0, 10000);
            initConfigTypeMap("Drug_PKPD_C50",                 &p_drug_model->drug_c50,                 GD_Drug_PKPD_C50_DESC_TEXT,                 0, 5000);
        }

        bool configured = BaseIntervention::Configure( inputJson );

        if( configured && !JsonConfigurable::_dryrun )
        {
            p_drug_model->PkPdParameterValidation();
        }

        return configured;
    }

    GenericDrug::GenericDrug( const std::string& rDefaultName, DrugModel* pDrugModel )
        : BaseIntervention()
        , p_drug_model( pDrugModel )
        , dosing_timer(0)
        , remaining_doses(1)
        , time_between_doses(0)
    {
        if( p_drug_model == nullptr )
        {
            p_drug_model = new DrugModel( rDefaultName );
        }
    }

    GenericDrug::GenericDrug( const GenericDrug& rThat )
        : BaseIntervention( rThat )
        , p_drug_model( nullptr )
        , dosing_timer( rThat.dosing_timer )
        , remaining_doses( rThat.remaining_doses )
        , time_between_doses( rThat.time_between_doses )
    {
        if( rThat.p_drug_model != nullptr )
        {
            this->p_drug_model = rThat.p_drug_model->Clone();
        }
    }

    GenericDrug::~GenericDrug()
    { 
        delete p_drug_model;
    }

    int
    GenericDrug::AddRef()
    {
        return BaseIntervention::AddRef();
    }

    int
    GenericDrug::Release()
    {
        return BaseIntervention::Release();
    }

    void
    GenericDrug::ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc )
    { 
        p_drug_model->ConfigureDrugTreatment( ivc );
    }

    const std::string&
    GenericDrug::GetDrugName() const
    {
        return p_drug_model->GetDrugName();
    }

    int GenericDrug::GetNumRemainingDoses() const
    {
        return remaining_doses;
    }

    float
    GenericDrug::GetDrugCurrentConcentration() const
    {
        return p_drug_model->GetDrugCurrentConcentration();
    }

    float
    GenericDrug::GetDrugCurrentEfficacy() const
    {
        return p_drug_model->GetDrugCurrentEfficacy();
    }

    float
    GenericDrug::GetDrugReducedAcquire()  const
    {
        return p_drug_model->GetDrugReducedAcquire();
    }

    float
    GenericDrug::GetDrugReducedTransmit() const
    {
        return p_drug_model->GetDrugReducedTransmit();
    }

    bool GenericDrug::Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO )
    {
        // shouldn't really need to do this specially since ConfigureDrugTreatment()
        // takes a context, but this will limit code changes.
        SetContextTo( context->GetParent() );

        ConfigureDrugTreatment( context );

        return BaseIntervention::Distribute( context, pICCO );
    }


    // I think we can get rid of this altogether
    void
    GenericDrug::Update(float dt)
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        // New Doses - remaining_doses = -1 implies infinite doses
        if( remaining_doses != 0 )
        {
            dosing_timer -= dt;
            LOG_DEBUG_F( "Remaining doses = %d, Dosing timer = %0.3f\n", remaining_doses, dosing_timer );
            if( (dosing_timer <= 0) && IsTakingDose( dt ) )
            {
                TakeDose( dt, parent->GetRng(), parent->GetInterventionsContext() );

                ResetForNextDose( dt );
            }
        }
        else if( remaining_doses == 0 )
        {
            if( !Expired() && (GetDrugCurrentEfficacy() <= FLT_EPSILON) ) // remaining_doses = -1 implies infinite doses
            {
                Expire();
            }
        }

        DecayAndUpdateEfficacy( dt );

        if( !expired )
        {
            ApplyEffects();
        }
    }

    void GenericDrug::TakeDose( float dt, RANDOMBASE* pRNG, IIndividualHumanInterventionsContext * ivc )
    {
        p_drug_model->TakeDose( dt, pRNG );
    }

    void GenericDrug::DecayAndUpdateEfficacy( float dt )
    {
        p_drug_model->DecayAndUpdateEfficacy( dt );
    }

    void GenericDrug::ResetForNextDose(float dt)
    {
        dosing_timer = time_between_doses;
        remaining_doses--;

        if (remaining_doses != 0 && time_between_doses < dt)
        {
            std::ostringstream oss;
            oss << "Time to next dose (" << time_between_doses << ") is shorter than the time-step, dt (" << dt << ")" << std::endl;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, oss.str().c_str() );
        }
    }

    // no-op
    void GenericDrug::ApplyEffects()
    {
    }

    void GenericDrug::Expire()
    {
        expired = true;
    }

    REGISTER_SERIALIZABLE(GenericDrug);

    void GenericDrug::serialize(IArchive& ar, GenericDrug* obj)
    {
        BaseIntervention::serialize( ar, obj );
        GenericDrug& drug = *obj;
        ar.labelElement( "p_drug_model"       ) & drug.p_drug_model;
        ar.labelElement( "dosing_timer"       ) & drug.dosing_timer;
        ar.labelElement( "remaining_doses"    ) & drug.remaining_doses;
        ar.labelElement( "time_between_doses" ) & drug.time_between_doses;
    }
}
