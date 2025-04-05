
#include "stdafx.h"
#include "MaleCircumcision.h"

#include "InterventionEnums.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanContext.h"
#include "NodeEventContext.h"
#include "ISTIInterventionsContainer.h"
#include "IIndividualHumanSTI.h"
#include "SimulationEnums.h"

SETUP_LOGGING( "MaleCircumcision" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(MaleCircumcision)
        HANDLE_INTERFACE( ICircumcision )
        HANDLE_INTERFACE( IConfigurable )
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(MaleCircumcision)

    IMPLEMENT_FACTORY_REGISTERED(MaleCircumcision)
    
    MaleCircumcision::MaleCircumcision()
    : BaseIntervention()
    , m_ReducedAcquire(1.0)
    , m_ApplyIfHigherReducedAcquire(false)
    , m_DistrbutedEventTrigger()
    , m_pCircumcisionConsumer(nullptr)
    , has_been_applied(false)
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
    }

    bool MaleCircumcision::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit, IV_Cost_To_Consumer_DESC_TEXT, 0.0f, FLT_MAX, 0.0f );
        initConfigTypeMap( "Circumcision_Reduced_Acquire", &m_ReducedAcquire, MaleCirc_Circumcision_Reduced_Acquire_DESC_TEXT, 0.0f, 1.0f, 0.60f );
        initConfigTypeMap( "Apply_If_Higher_Reduced_Acquire", &m_ApplyIfHigherReducedAcquire, MaleCirc_Apply_If_Higher_Reduced_Acquire_DESC_TEXT, false );

        initConfigTypeMap( "Distributed_Event_Trigger",
                           &m_DistrbutedEventTrigger,
                           Distributed_Event_Trigger_DESC_TEXT );

        return BaseIntervention::Configure( inputJson );
    }

    bool MaleCircumcision::Distribute( IIndividualHumanInterventionsContext *context,
                                       ICampaignCostObserver * const pCCO )
    {
        // ------------------------------------------
        // --- Get the CircumcisionConumer for later
        // ------------------------------------------
        if (s_OK != context->QueryInterface(GET_IID(ISTICircumcisionConsumer), (void**)&m_pCircumcisionConsumer) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ISTICircumcisionConsumer", "IIndividualHumanInterventionsContext" );
        }

        // -----------------------------------------------------------------
        // --- Make sure the the person is male and not already circumcised
        // -----------------------------------------------------------------
        IIndividualHumanSTI* p_sti = nullptr;
        if (s_OK != context->GetParent()->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&p_sti) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanSTI", "IIndividualHumanInterventionsContext" );
        }

        if( context->GetParent()->GetEventContext()->GetGender() == Gender::FEMALE )
        {
            LOG_WARN("Trying to circumcise females.\n");
            return false;
        }

        // --------------------------------------------------------------------------
        // --- If the man already has received the intervention, don't take any more.
        // --- First come, first accepted.
        // --- This avoid problem where he gets more than one intervention in the
        // --- same timestep.
        // !!! Added the !IsCircumcised() due to requirement to not expire!!!
        // --------------------------------------------------------------------------
        if( (context->GetInterventionsByInterface( GET_IID(ICircumcision) ).size() > 0) && !p_sti->IsCircumcised() )
        {
            return false;
        }

        if( p_sti->IsCircumcised() )
        {
            // -----------------------------------------------------------------------------------------------
            // --- If the individual is already circumcised,
            // --- we want to model the ability to get a better circumcision (i.e. m_ApplyIfHigherReducedAcquire),
            // --- and this intervention is a better one (higher reduced require), then
            // --- distribute the intervention.
            // --- The code is the negative because we want to return false.
            // -----------------------------------------------------------------------------------------------
            if( !m_ApplyIfHigherReducedAcquire || (m_ReducedAcquire <= m_pCircumcisionConsumer->GetCircumcisedReducedAcquire()) )
            {
                return false;
            }
        }

        // -------------------------------
        // --- Distribute the intervention
        // -------------------------------
        bool ret = BaseIntervention::Distribute( context, pCCO );

        // ----------------------------------------------------------------------------------
        // --- If the user defines a trigger, broadcast that the circumcision was distributed
        // ----------------------------------------------------------------------------------
        if( ret && !m_DistrbutedEventTrigger.IsUninitialized() )
        {
            IIndividualEventBroadcaster* broadcaster = context->GetParent()->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
            broadcaster->TriggerObservers( context->GetParent()->GetEventContext(), m_DistrbutedEventTrigger );
        }
        return ret;
    }

    void MaleCircumcision::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        if( !has_been_applied )
        {
            m_pCircumcisionConsumer->ApplyCircumcision( m_ReducedAcquire );
            has_been_applied = true;
        }
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! This should expire but ReferenceTrackingEventCoordinator needs
        // !!! the interventions to stay around.
        // !!! If changing to allow the intervention to expire, fix Distribute().
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //expired = true;
    }

    void MaleCircumcision::SetContextTo( IIndividualHumanContext *context )
    {
        BaseIntervention::SetContextTo( context );
        // ------------------------------------------
        // --- Get the CircumcisionConumer for later
        // ------------------------------------------
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(ISTICircumcisionConsumer), (void**)&m_pCircumcisionConsumer) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ISTICircumcisionConsumer", "IIndividualHumanInterventionsContext" );
        }
    }

    bool MaleCircumcision::ApplyIfHigherReducedAcquire() const
    {
        return m_ApplyIfHigherReducedAcquire;
    }

    float MaleCircumcision::GetReducedAcquire() const
    {
        return m_ReducedAcquire;
    }

    REGISTER_SERIALIZABLE(MaleCircumcision);

    void MaleCircumcision::serialize(IArchive& ar, MaleCircumcision* obj)
    {
        BaseIntervention::serialize( ar, obj );
        MaleCircumcision& mc = *obj;
        ar.labelElement("m_ReducedAcquire"             ) & mc.m_ReducedAcquire;
        ar.labelElement("m_ApplyIfHigherReducedAcquire") & mc.m_ApplyIfHigherReducedAcquire;
        ar.labelElement("m_DistrbutedEventTrigger"     ) & mc.m_DistrbutedEventTrigger;
    }
}
