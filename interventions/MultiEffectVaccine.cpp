
#include "stdafx.h"
#include "MultiEffectVaccine.h"
#include "InterventionsContainer.h"  // for IVaccineConsumer methods

SETUP_LOGGING( "MultiEffectVaccine" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(MultiEffectVaccine)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(IVaccine)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
    END_QUERY_INTERFACE_BODY(MultiEffectVaccine)

    IMPLEMENT_FACTORY_REGISTERED(MultiEffectVaccine)

    bool
    MultiEffectVaccine::Configure(
        const Configuration * inputJson
    )
    {
        WaningConfig acquire_config;
        WaningConfig transmit_config;
        WaningConfig mortality_config;

        initConfigTypeMap("Vaccine_Take", &vaccine_take, SV_Vaccine_Take_DESC_TEXT, 0.0, 1.0, 1.0 );
        initConfigComplexType("Acquire_Config",  &acquire_config, MEV_Acquire_Config_DESC_TEXT );
        initConfigComplexType("Transmit_Config",  &transmit_config, MEV_Transmit_Config_DESC_TEXT );
        initConfigComplexType("Mortality_Config", &mortality_config, MEV_Mortality_Config_DESC_TEXT  );

        bool configured = BaseIntervention::Configure( inputJson );
        if( configured && !JsonConfigurable::_dryrun )
        {
            acquire_effect   = WaningEffectFactory::getInstance()->CreateInstance( acquire_config._json,   inputJson->GetDataLocation(), "Acquire_Config"   );
            transmit_effect  = WaningEffectFactory::getInstance()->CreateInstance( transmit_config._json,  inputJson->GetDataLocation(), "Transmit_Config"  );
            mortality_effect = WaningEffectFactory::getInstance()->CreateInstance( mortality_config._json, inputJson->GetDataLocation(), "Mortality_Config" );
        }
        LOG_DEBUG_F( "Vaccine configured with type %d and take %f.\n", vaccine_type, vaccine_take );
        return configured;
    }

    MultiEffectVaccine::MultiEffectVaccine() 
    : SimpleVaccine()
    , acquire_effect( nullptr )
    , transmit_effect( nullptr )
    , mortality_effect( nullptr )
    {
    }

    MultiEffectVaccine::MultiEffectVaccine( const MultiEffectVaccine& master )
    : SimpleVaccine( master )
    , acquire_effect( nullptr )
    , transmit_effect( nullptr )
    , mortality_effect( nullptr )
    {
        vaccine_take = master.vaccine_take;
        cost_per_unit = master.cost_per_unit;

        acquire_effect   = master.acquire_effect->Clone();
        transmit_effect  = master.transmit_effect->Clone();
        mortality_effect = master.mortality_effect->Clone();
    }

    MultiEffectVaccine::~MultiEffectVaccine()
    {
        delete acquire_effect;
        delete transmit_effect;
        delete mortality_effect;
    }

    void MultiEffectVaccine::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        release_assert(ivc);

        acquire_effect->Update(dt);
        ivc->UpdateVaccineAcquireRate( acquire_effect->Current(), efficacy_is_multiplicative );

        transmit_effect->Update(dt);
        ivc->UpdateVaccineTransmitRate( transmit_effect->Current(), efficacy_is_multiplicative );

        mortality_effect->Update(dt);
        ivc->UpdateVaccineMortalityRate( mortality_effect->Current(), efficacy_is_multiplicative );

        if( !expired )
        {
            expired = acquire_effect->Expired() && transmit_effect->Expired() && mortality_effect->Expired();
        }
    }

    void MultiEffectVaccine::SetContextTo( IIndividualHumanContext *context )
    {
        SimpleVaccine::SetContextTo( context );
        acquire_effect->SetContextTo( context );
        transmit_effect->SetContextTo( context );
        mortality_effect->SetContextTo( context );
    }

    bool MultiEffectVaccine::NeedsInfectiousLoopUpdate() const
    {
        // ------------------------------------------------------------------------
        // --- Only mortality blocking impacts the infection directly so only
        // --- then does the intervention need to be in the infectious update loop.
        // ------------------------------------------------------------------------
        return (mortality_effect != nullptr);
    }

    REGISTER_SERIALIZABLE(MultiEffectVaccine);

    void MultiEffectVaccine::serialize(IArchive& ar, MultiEffectVaccine* obj)
    {
        SimpleVaccine::serialize( ar, obj );
        MultiEffectVaccine& vaccine = *obj;
        ar.labelElement("acquire_effect")   & vaccine.acquire_effect;
        ar.labelElement("transmit_effect")  & vaccine.transmit_effect;
        ar.labelElement("mortality_effect") & vaccine.mortality_effect;
    }
}
