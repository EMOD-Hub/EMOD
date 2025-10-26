
#include "stdafx.h"
#include "HumanHostSeekingTrap.h"

#include <typeinfo>

#include "IIndividualHumanContext.h"
#include "InterventionEnums.h"
#include "VectorInterventionsContainerContexts.h"  // for IVectorInterventionEffectsSetter methods

SETUP_LOGGING( "HumanHostSeekingTrap" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(HumanHostSeekingTrap)
        HANDLE_INTERFACE( IReportInterventionDataAccess )
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(HumanHostSeekingTrap)

    IMPLEMENT_FACTORY_REGISTERED(HumanHostSeekingTrap)

    HumanHostSeekingTrap::HumanHostSeekingTrap()
    : BaseIntervention()
    , killing_effect(nullptr)
    , attract_effect(nullptr)
    , ivies(nullptr)
    {
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0, 999999, 3.75 );
        
    }

    HumanHostSeekingTrap::~HumanHostSeekingTrap()
    {
        delete killing_effect;
        delete attract_effect;
    }

    HumanHostSeekingTrap::HumanHostSeekingTrap( const HumanHostSeekingTrap& master )
    : BaseIntervention( master )
    , killing_effect( nullptr )
    , attract_effect( nullptr )
    , ivies( nullptr )
    {
        if( master.killing_effect != nullptr )
        {
            killing_effect = master.killing_effect->Clone();
        }
        if( master.attract_effect != nullptr )
        {
            attract_effect = master.attract_effect->Clone();
        }
    }

    bool
    HumanHostSeekingTrap::Configure(
        const Configuration * inputJson
    )
    {
        WaningConfig   killing_config;
        WaningConfig   attract_config;

        initConfigComplexType("Killing_Config",  &killing_config, HST_Killing_Config_DESC_TEXT );
        initConfigComplexType("Attract_Config",  &attract_config, HST_Attract_Config_DESC_TEXT );

        bool configured = BaseIntervention::Configure( inputJson );

        if( !JsonConfigurable::_dryrun && configured )
        {
            killing_effect = WaningEffectFactory::getInstance()->CreateInstance( killing_config._json,
                                                                                 inputJson->GetDataLocation(),
                                                                                 "Killing_Config" );

            attract_effect = WaningEffectFactory::getInstance()->CreateInstance( attract_config._json,
                                                                                 inputJson->GetDataLocation(),
                                                                                 "Attract_Config" );
        }
        return configured;
    }

    bool
    HumanHostSeekingTrap::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pCCO
    )
    {
        if( AbortDueToDisqualifyingInterventionStatus( context->GetParent() ) )
        {
            return false;
        }
        // only one allowed at a time since the effects are assigned directly,
        // is not cumulative, and with multiples the last one wins
        context->PurgeExistingByName( typeid( *this ).name(), name );

        bool distributed = BaseIntervention::Distribute( context, pCCO );
        if( distributed )
        {
            if (s_OK != context->QueryInterface(GET_IID(IVectorInterventionEffectsSetter), (void**)&ivies) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVectorInterventionEffectsSetter", "IIndividualHumanInterventionsContext" );
            }
        }
        return distributed;
    }

    void HumanHostSeekingTrap::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        killing_effect->Update(dt);
        attract_effect->Update(dt);
        float current_killingrate = killing_effect->Current();
        float current_attractrate = attract_effect->Current();

        // Effects of human host-seeking trap are updated with indoor-home artificial-diet interfaces in VectorInterventionsContainer::Update.
        // Attraction rate diverts indoor feeding attempts from humans to trap; killing rate kills a fraction of diverted feeding attempts.
        ivies->UpdateArtificialDietAttractionRate( current_attractrate );
        ivies->UpdateArtificialDietKillingRate( current_killingrate );
    }

    ReportInterventionData HumanHostSeekingTrap::GetReportInterventionData() const
    {
        ReportInterventionData data = BaseIntervention::GetReportInterventionData();

        data.efficacy_attracting = attract_effect->Current();
        data.efficacy_killing    = killing_effect->Current();

        return data;
    }

    void HumanHostSeekingTrap::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        BaseIntervention::SetContextTo( context );
        killing_effect->SetContextTo( context );
        attract_effect->SetContextTo( context );

        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IVectorInterventionEffectsSetter), (void**)&ivies) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVectorInterventionEffectsSetter", "IIndividualHumanContext" );
        }
    }

    REGISTER_SERIALIZABLE(HumanHostSeekingTrap);

    void HumanHostSeekingTrap::serialize(IArchive& ar, HumanHostSeekingTrap* obj)
    {
        BaseIntervention::serialize( ar, obj );
        HumanHostSeekingTrap& trap = *obj;
        ar.labelElement("attract_effect") & trap.attract_effect;
        ar.labelElement("killing_effect") & trap.killing_effect;
    }
}

