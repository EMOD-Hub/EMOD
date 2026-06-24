
#include "stdafx.h"
#include "IndividualImmunityChanger.h"
#include "Debug.h" // for release_assert
#include "RANDOM.h"
#include "Common.h"             // for INFINITE_TIME
#include "IIndividualHuman.h"
#include "IIndividualHumanContext.h"
#include "InterventionsContainer.h"  // for IIndividualImmunityChangerEffects
#include "MathFunctions.h"  // for IIndividualImmunityChangerEffects
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)

SETUP_LOGGING( "IndividualImmunityChanger" );

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(IndividualImmunityChanger)

    BEGIN_QUERY_INTERFACE_BODY(IndividualImmunityChanger)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(IIndividualImmunityChanger)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(IndividualImmunityChanger)

    IndividualImmunityChanger::IndividualImmunityChanger()
        : BaseIntervention()
        , parent(nullptr)
        , isc(nullptr)
        , prime_acquire(0.0f)
        , prime_transmit(0.0f)
        , prime_mortality(0.0f)
        , boost_acquire(0.0f)
        , boost_transmit(0.0f)
        , boost_mortality(0.0f)
        , boost_threshold_acquire(0.0f)
        , boost_threshold_transmit(0.0f)
        , boost_threshold_mortality(0.0f)
        , update_acquire(0.0f)
        , update_transmit(0.0f)
        , update_mortality(0.0f)
    {
        expired = false;
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0, 999999, 1.0);
    }

    IndividualImmunityChanger::~IndividualImmunityChanger()
    {
        LOG_DEBUG("IndividualImmunityChanger destructor \n");
    }

    bool
    IndividualImmunityChanger::Configure(
        const Configuration * inputJson
    )
    {

        initConfigTypeMap("Prime_Acquire", &prime_acquire, MEBV_Prime_Acquire_DESC_TEXT, 0.0, 1.0, 0.0);
        initConfigTypeMap("Prime_Transmit", &prime_transmit, MEBV_Prime_Transmit_DESC_TEXT, 0.0, 1.0, 0.0);
        initConfigTypeMap("Prime_Mortality", &prime_mortality, MEBV_Prime_Mortality_DESC_TEXT, 0.0, 1.0, 0.0);
        initConfigTypeMap("Boost_Acquire", &boost_acquire, MEBV_Boost_Acquire_DESC_TEXT, 0.0, 1.0, 0.0);
        initConfigTypeMap("Boost_Transmit", &boost_transmit, MEBV_Boost_Transmit_DESC_TEXT, 0.0, 1.0, 0.0);
        initConfigTypeMap("Boost_Mortality", &boost_mortality, MEBV_Boost_Mortality_DESC_TEXT, 0.0, 1.0, 0.0);
        initConfigTypeMap("Boost_Threshold_Acquire", &boost_threshold_acquire, MEBV_Boost_Threshold_Acquire_DESC_TEXT, 0.0, 1.0, 0.0);
        initConfigTypeMap("Boost_Threshold_Transmit", &boost_threshold_transmit, MEBV_Boost_Threshold_Transmit_DESC_TEXT, 0.0, 1.0, 0.0);
        initConfigTypeMap("Boost_Threshold_Mortality", &boost_threshold_mortality, MEBV_Boost_Threshold_Mortality_DESC_TEXT, 0.0, 1.0, 0.0);
        bool ret = JsonConfigurable::Configure( inputJson );
        return ret;
    }

    bool
    IndividualImmunityChanger::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pCCO
    )
    {
        ApplyPrimingAndBoostingEffects(context);
        return BaseIntervention::Distribute( context, pCCO );
    }

    void IndividualImmunityChanger::ApplyPrimingAndBoostingEffects(IIndividualHumanInterventionsContext *context)
    {
        IDrugVaccineInterventionEffects* idvie = nullptr;
        if (s_OK != context->QueryInterface(GET_IID(IDrugVaccineInterventionEffects), (void**)&idvie))
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IDrugVaccineInterventionEffects", "IIndividualHumanInterventionsContext");
        }
        isc = context->GetParent()->GetSusceptibilityContext();

        update_acquire = 1.0f - prime_acquire;
        update_transmit = 1.0f - prime_transmit;
        update_mortality= 1.0f - prime_mortality;
        if ((idvie->GetInterventionReducedAcquire()*isc->getModAcquire()) < (1.0f - boost_threshold_acquire))
        {
            update_acquire = 1.0f - boost_acquire;
        }
        if ((idvie->GetInterventionReducedTransmit()* isc->getModTransmit()) < (1.0f - boost_threshold_transmit))
        {
            update_transmit = 1.0f - boost_transmit;
        }
        if ((idvie->GetInterventionReducedMortality()*isc->getModMortality()) < (1.0f - boost_threshold_mortality))
        {
            update_mortality = 1.0f - boost_mortality;
        }
    }

    void IndividualImmunityChanger::Update( float dt )
    {
        release_assert( expired == false );

        isc->updateModAcquire(update_acquire);
        isc->updateModTransmit(update_transmit);
        isc->updateModMortality(update_mortality);
        expired = true;
    }

    void IndividualImmunityChanger::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        parent = context;
    }

    int
    IndividualImmunityChanger::AddRef()
    {
        return BaseIntervention::AddRef();
    }

    int
    IndividualImmunityChanger::Release()
    {
        return BaseIntervention::Release();
    }


    REGISTER_SERIALIZABLE(IndividualImmunityChanger);

    void IndividualImmunityChanger::serialize(IArchive& ar, IndividualImmunityChanger* obj)
    {
        BaseIntervention::serialize( ar, obj );
        IndividualImmunityChanger& changer = *obj;

        ar.labelElement("prime_acquire"  ) & changer.prime_acquire;
        ar.labelElement("prime_transmit") & changer.prime_transmit;
        ar.labelElement("prime_mortality"          ) & changer.prime_mortality;
        ar.labelElement("boost_acquire"               ) & changer.boost_acquire;
        ar.labelElement("boost_transmit"         ) & changer.boost_transmit;
        ar.labelElement("boost_mortality"         ) & changer.boost_mortality;
        ar.labelElement("boost_threshold_acquire") & changer.boost_threshold_acquire;
        ar.labelElement("boost_threshold_transmit") & changer.boost_threshold_transmit;
        ar.labelElement("boost_threshold_mortality") & changer.boost_threshold_mortality;
        ar.labelElement("update_acquire") & changer.update_acquire;
        ar.labelElement("update_transmit") & changer.update_transmit;
        ar.labelElement("update_mortality") & changer.update_mortality;

    }
}
