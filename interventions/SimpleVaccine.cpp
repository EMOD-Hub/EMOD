
#include "stdafx.h"
#include "Vaccine.h"

#include "Common.h"                  // for INFINITE_TIME
#include "IIndividualHumanContext.h"
#include "InterventionsContainer.h"  // for IVaccineConsumer methods
#include "RANDOM.h"                  // for ApplyVaccineTake random draw

SETUP_LOGGING( "SimpleVaccine" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(SimpleVaccine)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE( IReportInterventionDataAccess )
        HANDLE_INTERFACE(IVaccine)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
    END_QUERY_INTERFACE_BODY(SimpleVaccine)

    IMPLEMENT_FACTORY_REGISTERED(SimpleVaccine)

    bool
    SimpleVaccine::Configure(
        const Configuration * inputJson
    )
    {
        WaningConfig waning_config;

        initConfigTypeMap("Vaccine_Take", &vaccine_take, SV_Vaccine_Take_DESC_TEXT, 0.0, 1.0, 1.0 );

        initConfig( "Vaccine_Type", vaccine_type, inputJson, MetadataDescriptor::Enum("Vaccine_Type", SV_Vaccine_Type_DESC_TEXT, MDD_ENUM_ARGS(SimpleVaccineType)));
        initConfigTypeMap("Efficacy_Is_Multiplicative", &efficacy_is_multiplicative, SV_Efficacy_Is_Multiplicative_DESC_TEXT, true );

        initConfigComplexType("Waning_Config",  &waning_config, SV_Waning_Config_DESC_TEXT );

        bool configured = BaseIntervention::Configure( inputJson );
        if( !JsonConfigurable::_dryrun )
        {
            waning_effect = WaningEffectFactory::getInstance()->CreateInstance( waning_config._json,
                                                                                inputJson->GetDataLocation(),
                                                                                "Waning_Config" );
        }
        LOG_DEBUG_F( "Vaccine configured with type %d and take %f.\n", vaccine_type, vaccine_take );
        return configured;
    }

    SimpleVaccine::SimpleVaccine() 
    : BaseIntervention()
    , vaccine_type(SimpleVaccineType::Generic)
    , vaccine_take(0.0)
    , vaccine_took(false)
    , efficacy_is_multiplicative(true)
    , waning_effect( nullptr )
    , ivc( nullptr )
    {
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0, 999999, 10.0);
    }

    SimpleVaccine::SimpleVaccine( const SimpleVaccine& master )
    : BaseIntervention( master )
    , vaccine_type(master.vaccine_type)
    , vaccine_take(master.vaccine_take)
    , vaccine_took(master.vaccine_took)
    , efficacy_is_multiplicative(master.efficacy_is_multiplicative )
    , waning_effect( nullptr )
    , ivc( nullptr )
    {
        if( master.waning_effect != nullptr )
        {
            waning_effect = master.waning_effect->Clone();
        }
    }

    SimpleVaccine::~SimpleVaccine()
    {
        delete waning_effect;
        waning_effect = nullptr;
    }

    /////////////////////////////////////////////////////////////////////////////////////

    // context is nothing more than ISupports really, and it's a pointer to the individual's
    // intervention container, not the individual itself. It was gotten by a call to
    // pIndividual->GetInterventionsContext().

    bool
    SimpleVaccine::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        // store ivc for apply
        LOG_DEBUG("Distributing SimpleVaccine.\n");
        if (s_OK != context->QueryInterface(GET_IID(IVaccineConsumer), (void**)&ivc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVaccineConsumer", "IIndividualHumanInterventionsContext" );
        }

        // ---------------------------------------------------------------------------------------------
        // --- One does not know if a vaccine 'took' or not unless they go through a specific test
        // --- to see if they have developed the correct antibodies.  Normally, once they have received
        // --- a vaccine, they are considered vaccinated and are counted in the vaccinated group.
        // --- However, if the vaccine did not take, we need to have the vaccine be ineffective.
        // ---------------------------------------------------------------------------------------------
        vaccine_took = ApplyVaccineTake( context->GetParent() );

        bool distribute =  BaseIntervention::Distribute( context, pCCO );
        return distribute;
    }

    void SimpleVaccine::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        // -----------------------------------------------------------------
        // --- Still update waning_effect even if the vaccine did not take.
        // --- This allows it to expire on schedule.
        // -----------------------------------------------------------------
        waning_effect->Update(dt);

        // ----------------------------------------------------------------------
        // --- If the vaccine did not take, do not attempt to update the vaccine
        // --- behavior within the individual.  
        // ----------------------------------------------------------------------
        if( vaccine_took )
        {
            release_assert(ivc);
            switch( vaccine_type )
            {
                case SimpleVaccineType::AcquisitionBlocking:
                    ivc->UpdateVaccineAcquireRate( waning_effect->Current(), efficacy_is_multiplicative );
                    break;

                case SimpleVaccineType::TransmissionBlocking:
                    ivc->UpdateVaccineTransmitRate( waning_effect->Current(), efficacy_is_multiplicative );
                    break;

                case SimpleVaccineType::MortalityBlocking:
                    ivc->UpdateVaccineMortalityRate( waning_effect->Current(), efficacy_is_multiplicative );
                    break;

                case SimpleVaccineType::Generic:
                    ivc->UpdateVaccineAcquireRate(   waning_effect->Current(), efficacy_is_multiplicative );
                    ivc->UpdateVaccineTransmitRate(  waning_effect->Current(), efficacy_is_multiplicative );
                    ivc->UpdateVaccineMortalityRate( waning_effect->Current(), efficacy_is_multiplicative );
                    break;

                default:
                    throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "vaccine_type", vaccine_type, SimpleVaccineType::pairs::lookup_key( vaccine_type ) );
                    break;
            }
        }

        if( !expired )
        {
            expired = waning_effect->Expired();
        }
    }

    ReportInterventionData SimpleVaccine::GetReportInterventionData() const
    {
        ReportInterventionData data = BaseIntervention::GetReportInterventionData();

        if( vaccine_took )
        {
            if( (vaccine_type == SimpleVaccineType::AcquisitionBlocking) ||
                (vaccine_type == SimpleVaccineType::Generic) )
            {
                data.efficacy_acq = waning_effect->Current();
            }
            if( (vaccine_type == SimpleVaccineType::TransmissionBlocking) ||
                (vaccine_type == SimpleVaccineType::Generic) )
            {
                data.efficacy_tran = waning_effect->Current();
            }
            if( (vaccine_type == SimpleVaccineType::MortalityBlocking) ||
                (vaccine_type == SimpleVaccineType::Generic) )
            {
                data.efficacy_mort = waning_effect->Current();
            }
        }

        return data;
    }

    bool SimpleVaccine::ApplyVaccineTake( IIndividualHumanContext* pihc )
    {
        release_assert( pihc );

        bool did_vaccine_take = pihc->GetRng()->SmartDraw( vaccine_take );
        return did_vaccine_take;
    }

    void SimpleVaccine::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        BaseIntervention::SetContextTo( context );
        if( waning_effect != nullptr )
        {
            waning_effect->SetContextTo( context );
        }

        if (s_OK != parent->GetInterventionsContext()->QueryInterface(GET_IID(IVaccineConsumer), (void**)&ivc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context->GetInterventionsContext()", "IVaccineConsumer", "IIndividualHumanInterventionsContext" );
        }
        LOG_DEBUG_F( "Vaccine configured with type %d and take %f for individual %d\n", vaccine_type, vaccine_take, parent->GetSuid().data );
    } // needed for VaccineTake

    bool SimpleVaccine::NeedsInfectiousLoopUpdate() const
    {
        // ------------------------------------------------------------------------
        // --- Only mortality blocking impacts the infection directly so only
        // --- then does the intervention need to be in the infectious update loop.
        // ------------------------------------------------------------------------
        return (vaccine_type == SimpleVaccineType::MortalityBlocking) ||
               (vaccine_type == SimpleVaccineType::Generic          );
    }


    REGISTER_SERIALIZABLE(SimpleVaccine);

    void SimpleVaccine::serialize(IArchive& ar, SimpleVaccine* obj)
    {
        BaseIntervention::serialize( ar, obj );
        SimpleVaccine& vaccine = *obj;
        ar.labelElement("vaccine_type"              ) & vaccine.vaccine_type;
        ar.labelElement("vaccine_take"              ) & vaccine.vaccine_take;
        ar.labelElement("vaccine_took"              ) & vaccine.vaccine_took;
        ar.labelElement("efficacy_is_multiplicative") & vaccine.efficacy_is_multiplicative;
        ar.labelElement("waning_effect"             ) & vaccine.waning_effect;
    }
}
