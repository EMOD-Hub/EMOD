
#include "stdafx.h"
#include "Relationship.h"
#include "Debug.h"
#include "Environment.h"
#include "Types.h"
#include "MathFunctions.h"
#include "RANDOM.h"
#include "INodeContext.h"
#include "STIInterventionsContainer.h" // for ISTIBarrierConsumer (which should be in ISTIBarrierConsumer.h) TODO
#include "IndividualSTI.h"
#include "ISociety.h"
#include "IdmString.h"

#include <map>
#include <ctime>
#include <algorithm>

SETUP_LOGGING( "Relationship" )

namespace Kernel {

    // static
    bool alreadyInRelationship(
        IIndividualHumanSTI * pGuy,
        IIndividualHumanSTI * pGal
    )
    {
        auto guyRels = pGuy->GetRelationships();
        for( const auto relationship : guyRels)
        {
            auto partner = relationship->GetPartner( pGuy );
            if( partner && (partner->GetSuid().data == pGal->GetSuid().data) )
            {
                return true;
            }
        }
        return false;
    }

#define MALE_PARTNER_ID()       ( (absent_male_partner_id   == suids::nil_suid()) ? male_partner->GetSuid()   : absent_male_partner_id   )
#define FEMALE_PARTNER_ID()     ( (absent_female_partner_id == suids::nil_suid()) ? female_partner->GetSuid() : absent_female_partner_id )
#define PARTNERID( individual ) ( GetPartner( individual ) ? GetPartner( individual )->GetSuid() : suids::nil_suid()        )

    BEGIN_QUERY_INTERFACE_BODY(Relationship)
        HANDLE_INTERFACE(IRelationship)
        HANDLE_ISUPPORTS_VIA(IRelationship)
    END_QUERY_INTERFACE_BODY(Relationship)


    Relationship::Relationship()
        : IRelationship()
        , _suid(suids::nil_suid())
        , is_outside_pfa(false)
        , state( RelationshipState::NORMAL )
        , previous_state( RelationshipState::NORMAL )
        , relationship_type( RelationshipType::TRANSITORY )
        , termination_reason( RelationshipTerminationReason::NA )
        , p_rel_params( nullptr )
        , male_partner(nullptr)
        , female_partner(nullptr)
        , absent_male_partner_id(suids::nil_suid())
        , absent_female_partner_id(suids::nil_suid())
        , rel_timer(0)
        , rel_duration(0)
        , start_time(0)
        , scheduled_end_time(0)
        , original_node_id(0)
        , act_prob_vec()
        , total_coital_acts(0)
        , coital_acts_this_dt(0)
        , coital_acts_this_dt_using_condoms(0)
        , has_migrated(false)
        , migration_destination()
        , male_slot_number(UINT_MAX)
        , female_slot_number(UINT_MAX)
        , p_condom_usage( nullptr )
    {
    }

    Relationship::Relationship( const Relationship& rMaster)
        : IRelationship( rMaster )
        , _suid( rMaster._suid )
        , is_outside_pfa( rMaster.is_outside_pfa )
        , state( rMaster.state )
        , previous_state( rMaster.previous_state )
        , relationship_type( rMaster.relationship_type )
        , termination_reason( rMaster.termination_reason )
        , p_rel_params( rMaster.p_rel_params )
        , male_partner( rMaster.male_partner )
        , female_partner( rMaster.female_partner )
        , absent_male_partner_id( rMaster.absent_male_partner_id )
        , absent_female_partner_id( rMaster.absent_female_partner_id )
        , rel_timer( rMaster.rel_timer )
        , rel_duration( rMaster.rel_duration )
        , start_time( rMaster.start_time )
        , scheduled_end_time( rMaster.scheduled_end_time )
        , original_node_id( rMaster.original_node_id )
        , act_prob_vec( rMaster.act_prob_vec )
        , total_coital_acts( rMaster.total_coital_acts )
        , coital_acts_this_dt( rMaster.coital_acts_this_dt )
        , coital_acts_this_dt_using_condoms( rMaster.coital_acts_this_dt_using_condoms )
        , has_migrated( rMaster.has_migrated )
        , migration_destination( rMaster.migration_destination )
        , male_slot_number( rMaster.male_slot_number )
        , female_slot_number( rMaster.female_slot_number )
        , p_condom_usage( nullptr )
    {
        if( rMaster.p_condom_usage != nullptr )
        {
            this->p_condom_usage = new Sigmoid( *rMaster.p_condom_usage );
        }
    }

    Relationship::Relationship( RANDOMBASE* pRNG,
                                const suids::suid& rRelId,
                                IRelationshipParameters* pParams,
                                IIndividualHumanSTI * male_partnerIn, 
                                IIndividualHumanSTI * female_partnerIn,
                                bool isOutsidePFA,
                                Sigmoid* pCondomUsage )
        : IRelationship()
        , _suid(rRelId)
        , is_outside_pfa( isOutsidePFA )
        , state( RelationshipState::NORMAL )
        , previous_state( RelationshipState::NORMAL )
        , relationship_type( pParams->GetType() )
        , termination_reason( RelationshipTerminationReason::NA )
        , p_rel_params( pParams )
        , male_partner(male_partnerIn)
        , female_partner(female_partnerIn)
        , absent_male_partner_id(suids::nil_suid())
        , absent_female_partner_id(suids::nil_suid())
        , rel_timer(0)
        , rel_duration(0)
        , start_time(0)
        , scheduled_end_time(0)
        , original_node_id(0)
        , act_prob_vec()
        , total_coital_acts(0)
        , coital_acts_this_dt(0)
        , coital_acts_this_dt_using_condoms(0)
        , has_migrated(false)
        , migration_destination()
        , male_slot_number(UINT_MAX)
        , female_slot_number(UINT_MAX)
        , p_condom_usage( nullptr )
    {
        release_assert( male_partner   );
        release_assert( female_partner );
        release_assert( male_partner->GetIndividualHuman()->GetGender()   == Gender::MALE   );
        release_assert( female_partner->GetIndividualHuman()->GetGender() == Gender::FEMALE );

        LOG_DEBUG_F( "%s: Creating relationship %d between %d and %d\n", __FUNCTION__,
                     _suid.data, MALE_PARTNER_ID().data, FEMALE_PARTNER_ID().data );

        rel_timer = DAYSPERYEAR * pRNG->Weibull2( pParams->GetDurationWeibullScale(),
                                                  pParams->GetDurationWeibullHeterogeneity() );

        IIndividualHuman* individual = male_partnerIn->GetIndividualHuman();

        start_time = float(individual->GetParent()->GetTime().time);
        scheduled_end_time = start_time + rel_timer;

        original_node_id = individual->GetParent()->GetExternalID();

        if( !is_outside_pfa )
        {
            male_slot_number   = male_partner->GetOpenRelationshipSlot();
            female_slot_number = female_partner->GetOpenRelationshipSlot();
        }

        male_partner->AddRelationship( this );
        female_partner->AddRelationship( this );

        if( pCondomUsage != nullptr )
        {
            this->p_condom_usage = new Sigmoid( *pCondomUsage );
        }
    }

    Relationship::~Relationship()
    {
        LOG_DEBUG_F( "(EEL) relationship %d between %d and %d just ended.\n", GetSuid().data, MALE_PARTNER_ID().data, FEMALE_PARTNER_ID().data );
        // ------------------------------------------------------
        // --- Do not own p_rel_params so don't delete
        // ------------------------------------------------------

        delete p_condom_usage;
    }

    Relationship* Relationship::Clone()
    {
        return new Relationship( *this );
    }

    bool Relationship::IsOutsidePFA() const
    {
        return is_outside_pfa;
    }

    RelationshipState::Enum Relationship::GetState() const
    {
        return state;
    }

    RelationshipState::Enum Relationship::GetPreviousState() const
    {
        return previous_state;
    }

    RelationshipMigrationAction::Enum Relationship::GetMigrationAction( RANDOMBASE* prng ) const
    {
        if( state == RelationshipState::PAUSED )
        {
            return RelationshipMigrationAction::PAUSE;
        }
        else if( state == RelationshipState::MIGRATING )
        {
            return RelationshipMigrationAction::MIGRATE;
        }
        else if( state == RelationshipState::TERMINATED )
        {
            std::ostringstream msg;
            msg << "Should not be trying to get a migration action when the relationship has been terminated. rel_id=" << GetSuid().data ;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        else
        {
            release_assert( state == RelationshipState::NORMAL );

            const std::vector<RelationshipMigrationAction::Enum>& r_actions = p_rel_params->GetMigrationActions();

            if( r_actions.size() == 1 )
            {
                return r_actions[0];
            }
            else
            {
                const std::vector<float>& r_cdf = p_rel_params->GetMigrationActionsCDF();
                release_assert( r_actions.size() == r_cdf.size() );

                float ran = prng->e();
                for( int i = 0 ; i < r_cdf.size() ; i++)
                {
                    if( r_cdf[i] >= ran )
                    {
                        return r_actions[i];
                    }
                }
                std::ostringstream msg;
                msg << "Should have selected an action.  Is the last value not 1.0? ran=" << ran << " last_value=" << r_cdf[ r_cdf.size()-1 ] ;
                throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
        }
    }

    RelationshipTerminationReason::Enum Relationship::GetTerminationReason() const
    {
        return termination_reason;
    }

    void Relationship::SetParameters( ISociety* pSociety )
    {
        if( pSociety == nullptr )
        {
            p_rel_params = nullptr ;
        }
        else
        {
            p_rel_params = pSociety->GetRelationshipParameters( relationship_type );
        }
    }

    bool
    Relationship::Update( float dt )
    {
        coital_acts_this_dt = 0;
        coital_acts_this_dt_using_condoms = 0;
        LOG_VALID_F( "%s: man = %d, woman = %d, timer getting decremented from %f to %f\n", __FUNCTION__, MALE_PARTNER_ID().data, FEMALE_PARTNER_ID().data, rel_timer, rel_timer - 1 );

        rel_duration += dt;
        rel_timer -= dt;
        if( rel_timer <= 0 )
        {
            LOG_DEBUG_F( "%s: relationship %d between %d and %d is over, timer = %f...\n", __FUNCTION__, GetSuid().data, MALE_PARTNER_ID().data, FEMALE_PARTNER_ID().data, rel_timer );
            return false;
        }
        return true;
    }

    void Relationship::Consummate( RANDOMBASE* pRNG, float dt, IRelationshipManager* pRelMan )
    {
        if( (state != RelationshipState::NORMAL) ||
            (male_partner->GetIndividualHuman()->GetAge()   < 12*DAYSPERYEAR) ||
            (female_partner->GetIndividualHuman()->GetAge() < 12*DAYSPERYEAR) )
        {
            return;
        }
        release_assert( pRelMan );

        // DJK: If we don't care about logging individual coital acts, could check IsDiscordant here
        unsigned int nRels = 1;
        float coital_rate_attenuation_factor = 1;
        if( IndividualHumanSTIConfig::enable_coital_dilution )
        {
            nRels = std::max<size_t>( male_partner->GetRelationships().size(), female_partner->GetRelationships().size() );

            if( nRels == 2)
                coital_rate_attenuation_factor = IndividualHumanSTIConfig::coital_dilution_2_partners;
            else if( nRels == 3)
                coital_rate_attenuation_factor = IndividualHumanSTIConfig::coital_dilution_3_partners;
            else if( nRels >= 4)
                coital_rate_attenuation_factor = IndividualHumanSTIConfig::coital_dilution_4_plus_partners;

            if( nRels > 1 )
            {
                LOG_DEBUG_F( "Coital dilution in effect: nRels is %d so attenuating coital rate by %f\n", nRels, coital_rate_attenuation_factor );
            }
        }
        release_assert( coital_rate_attenuation_factor > 0 );

        /*LOG_DEBUG_F( "(EEL) Coital Act: rel = %d, insertive = %d, relationships = %d, receptive = %d, relationships = %d, time till next act = %f.\n",
                    GetSuid().data,
                    male_partner->GetSuid().data,
                    male_partner->GetRelationships().size(),
                    female_partner->GetSuid().data,
                    female_partner->GetRelationships().size(),
                    next_coital_act_timer
                  );*/

        double ratetime = dt * coital_rate_attenuation_factor * GetCoitalRate();
        coital_acts_this_dt = pRNG->Poisson( ratetime );

        if( total_coital_acts == 0 && coital_acts_this_dt == 0 && ratetime > 0)
        {
            coital_acts_this_dt = 1;   // Force the first act
        }

        total_coital_acts += coital_acts_this_dt;

        // ----------------------------------------------------------------------------------------------------
        // --- We get the infected state here so that it can't change.  When calling UpdateNumCoitalActs(),
        // --- we can broadcast the FirstCoitalAct event.  A user could distribute OutbreakIndividual for that
        // --- event causing the person to become newly infected.  This protectes against that rare case.
        // ----------------------------------------------------------------------------------------------------
        bool is_infected_male   = male_partner->IsInfected();
        bool is_infected_female = female_partner->IsInfected();

        // if neither are infected, we end up with the male being "infected" and female being "uninfected"
        IIndividualHumanSTI *uninfected_individual = is_infected_male ? female_partner : male_partner;
        IIndividualHumanSTI *infected_individual   = is_infected_male ? male_partner   : female_partner;

        // STI risk factor if one partner has sti, avoid double risk
        float coInfectiveFactor_tran = infected_individual->GetCoInfectiveTransmissionFactor();
        float coInfectiveFactor_acq = uninfected_individual->GetCoInfectiveAcquisitionFactor();
        float risk_mult = max( coInfectiveFactor_tran, coInfectiveFactor_acq );

        float risk_factor_tran = infected_individual->GetCoitalActRiskTransmissionFactor();
        float risk_factor_acq = uninfected_individual->GetCoitalActRiskAcquisitionFactor();

        risk_mult *= risk_factor_tran * risk_factor_acq;

        // ------------------------------------------------------------------------------
        // --- Loop through each coital act and registering each act with the individuals
        // --- so that we can later determine which act had transmission.
        // ------------------------------------------------------------------------------
        ProbabilityNumber p_condom = getProbabilityUsingCondomThisAct();
        bool any_condomless_sex = false;
        for( uint32_t iact = 0; iact < coital_acts_this_dt; ++iact )
        {
            male_partner->UpdateNumCoitalActs( 1 );
            female_partner->UpdateNumCoitalActs( 1 );

            bool is_using_condom = pRNG->SmartDraw( p_condom );    // For coital act reporting

            float act_risk = risk_mult;
            if( is_using_condom )
            {
                act_risk *= (1.0 - IndividualHumanSTIConfig::condom_transmission_blocking_probability);
                ++coital_acts_this_dt_using_condoms;
            }
            else 
            {
                any_condomless_sex = true;
            }

            CoitalAct act( pRelMan->GetNextCoitalActSuid(),
                           GetSuid(),
                           uninfected_individual->GetSuid(),
                           infected_individual->GetSuid(),
                           is_using_condom,
                           act_risk );

            pRelMan->ConsummateRelationship( this, act );

            // Only do work if couple is discordant
            if( is_infected_male != is_infected_female )
            {
                uninfected_individual->NotifyPotentialExposure();
                infected_individual->UpdateInfectiousnessSTI( act );
            }
        }
        if (any_condomless_sex) {
            if (relationship_type == RelationshipType::MARITAL) {
                male_partner->GetIndividualHuman()->BroadcastEvent(EventTrigger::MaritalCondomlessSexAct);
                female_partner->GetIndividualHuman()->BroadcastEvent(EventTrigger::MaritalCondomlessSexAct);
            }
            else 
            {
                male_partner->GetIndividualHuman()->BroadcastEvent(EventTrigger::NonMaritalCondomlessSexAct);
                female_partner->GetIndividualHuman()->BroadcastEvent(EventTrigger::NonMaritalCondomlessSexAct);
            }
        }
    }

    IIndividualHumanSTI* Relationship::MalePartner() const
    {
        return male_partner;
    }

    IIndividualHumanSTI* Relationship::FemalePartner()    const
    {
        return female_partner;
    }

    IIndividualHumanSTI*
    Relationship::GetPartner(
        IIndividualHumanSTI* individual
    )
    const
    {
        if ( individual == nullptr )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "GetPartner called with null pointer.\n" );
        }

        IIndividualHumanSTI* partner = nullptr;
        if (individual == male_partner)
        {
            partner = female_partner;
        }
        else if (individual == female_partner)
        {
            partner = male_partner;
        }
        else
        {
            std::ostringstream msg;
            msg << "Individual "<< individual->GetSuid().data << " is not in relationship " << GetSuid().data << "!\n";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        // ----------------------------------------------------------------------------
        // --- We allow the return to be null.  If the relationship is paused and the
        // --- individual moved nodes, then the pointer could be null.
        // ----------------------------------------------------------------------------

        return partner;
    }

    const suids::suid& Relationship::GetSuid() const
    {
        return _suid;
    }

    unsigned int Relationship::GetSlotNumberForPartner( bool forPartnerB ) const
    {
        // Note: this solution assumes max of MAX_SLOT relationships
        if( forPartnerB ) // B=Female
        {
            return female_slot_number;
        }
        else
        {
            return male_slot_number;
        }
    }

    const tRelationshipMembers
    Relationship::GetMembers() const
    {
        tRelationshipMembers returnSet;
        returnSet.insert( male_partner );
        returnSet.insert( female_partner );
        return returnSet;
    }

    IRelationship* Relationship::Pause( IIndividualHumanSTI* departee, const suids::suid& rMigrationDestination )
    {
        LOG_DEBUG_F( "%s: individual %d vacating relationship %d leaving individual %d alone.\n", __FUNCTION__, departee->GetSuid().data, GetSuid().data, PARTNERID( departee ).data );

        previous_state = state;
        state = RelationshipState::PAUSED;
        migration_destination = rMigrationDestination;

        // -------------------------------------------------------------------------------
        // --- When Pausing, we need to create two instances of the relationship: 
        // --- one for the current node and one for the node the person is going to.
        // --- One reason for this is that Relationship::Update() is called by RelationshipManager::Update()
        // --- so that it is called once for the relationship.  Since we need two instances
        // --- for multi-core, it makes sense to use this solution for single core as well.
        // -------------------------------------------------------------------------------
        Relationship* p_staying_rel = this;
        Relationship* p_leaving_rel = nullptr;
        if( previous_state == RelationshipState::NORMAL )
        {
            p_leaving_rel = this->Clone();
            std::vector<IRelationship*>& r_relationships = departee->GetRelationships();
            for( int i = 0; i < r_relationships.size(); ++i )
            {
                if( r_relationships[ i ] == p_staying_rel )
                {
                    r_relationships[ i ] = p_leaving_rel;
                    break;
                }
            }
        }
        else
        {
            release_assert( previous_state == RelationshipState::PAUSED );
            p_leaving_rel = p_staying_rel;
        }

        // -------------------------------------------------------------------------
        // --- When Pausing a relationship, each partner will have their own instance
        // --- of the relationship object.  This implies that we need these objects setup
        // --- so that the relationship object for Partner A (male) has himself still in
        // --- the relationship and his partner absent.  Partner B needs the opposite.
        // ---
        // --- We don't set both partners to absent since we want the actions of one
        // --- partner to reset the relationship.  For example, if one partner pauses
        // --- relationship due to migration, and then returns.  That individual's call
        // --- to Resume() should get the relationship back to normal.
        // ---
        // --- Not setting absent partner to null so that the pointer can be used in the Transmission report.
        // --- The couple could have consumated and then one of them decided to pause the relationship.
        // --- The report needs the pointer.
        // -------------------------------------------------------------------------
        if( p_staying_rel->male_partner == departee )
        {
            LOG_DEBUG_F( "%s: departee was male_partner, clearing male_partner from relationship %d\n", __FUNCTION__, p_staying_rel->GetSuid().data );

            p_staying_rel->absent_male_partner_id = departee->GetSuid();

            if( (p_leaving_rel != nullptr) && (previous_state == RelationshipState::NORMAL))
            {
                release_assert( p_staying_rel->female_partner );
                p_leaving_rel->absent_female_partner_id = p_staying_rel->female_partner->GetSuid();
                p_leaving_rel->absent_male_partner_id = p_staying_rel->male_partner->GetSuid();
            }
        }
        else if( p_staying_rel->female_partner == departee )
        {
            LOG_DEBUG_F( "%s: departee was female_partner, clearing female_partner from relationship %d\n", __FUNCTION__, GetSuid().data );

            p_staying_rel->absent_female_partner_id = departee->GetSuid();

            if( (p_leaving_rel != nullptr) && (previous_state == RelationshipState::NORMAL) )
            {
                release_assert( p_staying_rel->male_partner );
                p_leaving_rel->absent_female_partner_id = p_staying_rel->female_partner->GetSuid();
                p_leaving_rel->absent_male_partner_id = p_staying_rel->male_partner->GetSuid();
            }
        }
        else
        {
            std::ostringstream msg;
            msg << "Individual "<< departee->GetSuid().data << " is not in relationship " << GetSuid().data << "!\n";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
            
        return p_leaving_rel;
    }

    void Relationship::UpdatePaused()
    {
        // ----------------------------------------------------------------------------------------------
        // --- If the relationship is paused, we want to set the partner pointers to null 
        // --- so that they are not accidentally used later when they are not valid.
        // --- We don't set them to null in Pause() because we need the pointers to be
        // --- valid for the Transmission report.  Setting the pointers to null at the end
        // --- of Individual::Update() is needed for the situation where partner A updates, 
        // --- infects partner B, and decides to pause the relationship.  Partner B won't update 
        // --- the Transmission report until his update.  At this point, we need the pointers in
        // --- relationship to be available for when partner B becomes infected and updates the report.
        // ----------------------------------------------------------------------------------------------
        if( (state == RelationshipState::PAUSED) || (state == RelationshipState::MIGRATING) )
        {
            if( absent_male_partner_id != suids::nil_suid() )
            {
                male_partner = nullptr;
            }
            if( absent_female_partner_id != suids::nil_suid() )
            {
                female_partner = nullptr;
            }
        }
    }

    void Relationship::Resume( ISociety* pSociety, 
                               IIndividualHumanSTI* returnee )
    {
        release_assert( pSociety != nullptr );
        release_assert( returnee != nullptr );
        migration_destination = suids::nil_suid();

        if( state == RelationshipState::TERMINATED )
        {
            std::ostringstream msg;
            msg << "RelationshipId=" << GetSuid().data << " has already been terminated.  It cannot be resumed." ;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        auto returneeId = returnee->GetSuid();
        if( (absent_male_partner_id == returneeId) || (male_partner == returnee) )
        {
            absent_male_partner_id = suids::nil_suid();
            male_partner = returnee;
            LOG_DEBUG_F( "%s: individual %d returning to relationship %d with individual %d \n", __FUNCTION__, returnee->GetSuid().data, GetSuid().data, PARTNERID( returnee ).data );
            LOG_DEBUG_F( "%s: returnee is male_partner, restoring male_partner to relationship %d\n", __FUNCTION__, GetSuid().data );
        }
        else if( (absent_female_partner_id == returneeId) || (female_partner == returnee) )
        {
            absent_female_partner_id = suids::nil_suid();
            female_partner = returnee;
            LOG_DEBUG_F( "%s: individual %d returning to relationship %d with individual %d \n", __FUNCTION__, returnee->GetSuid().data, GetSuid().data, PARTNERID( returnee ).data );
            LOG_DEBUG_F( "%s: returnee is female_partner, restoring female_partner to relationship %d\n", __FUNCTION__, GetSuid().data );
        }
        else
        {
            std::ostringstream msg;
            msg << "Unknown partner:  rel_id=" << GetSuid().data << " male_id=" << absent_male_partner_id.data << "  female_id=" << absent_female_partner_id.data << " returnee_id=" << returneeId.data ;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        if( (absent_male_partner_id == suids::nil_suid()) && (absent_female_partner_id == suids::nil_suid()) )
        {
            release_assert( male_partner );
            release_assert( female_partner );
            if( male_partner->GetNodeSuid() == female_partner->GetNodeSuid() )
            {
                previous_state = state;
                state = RelationshipState::NORMAL;
            }
        }
        SetParameters( pSociety );
    }

    void Relationship::Terminate( RelationshipTerminationReason::Enum reason )
    {
        LOG_DEBUG_F( "%s: terminating relationship %d:%x with state=%s between individual %d and individual %d because %s.\n",
            __FUNCTION__,
            GetSuid().data,
            this, 
            RelationshipState::pairs::lookup_key( state ),
            MALE_PARTNER_ID().data, 
            FEMALE_PARTNER_ID().data,
            RelationshipTerminationReason::pairs::lookup_key( reason ));

        // -----------------------------------------------------------------------
        // --- Migrating would have set these values so we want to clear them to
        // --- make sure it is clear that the relationship has been terminated.
        // -----------------------------------------------------------------------
        if( state == RelationshipState::MIGRATING )
        {
            absent_male_partner_id   = suids::nil_suid();
            absent_female_partner_id = suids::nil_suid();
        }

        previous_state = state;
        state = RelationshipState::TERMINATED;
        termination_reason = reason;

        SetParameters( nullptr );

        if( (male_partner != nullptr) && (absent_male_partner_id == suids::nil_suid()) )
        { 
            male_partner->RemoveRelationship( this );
            // ----------------------------------------------------------
            // --- We don't set this to nullptr so that the relationship
            // --- can be used later to collect data at death
            // ----------------------------------------------------------
            //male_partner = nullptr;
        }

        if( (female_partner != nullptr) && (absent_female_partner_id == suids::nil_suid()) )
        {
            female_partner->RemoveRelationship( this );
            // ----------------------------------------------------------
            // --- We don't set this to nullptr so that the relationship
            // --- can be used later to collect data at death
            // ----------------------------------------------------------
            //female_partner = nullptr;
        }
    }

    void Relationship::Migrate( const suids::suid& rMigrationDestination )
    {
        migration_destination = rMigrationDestination;
        has_migrated = true;

        if( state == RelationshipState::NORMAL )
        {
            previous_state = state;
            state = RelationshipState::MIGRATING;

            absent_male_partner_id   = male_partner->GetSuid();
            absent_female_partner_id = female_partner->GetSuid();

            // -------------------------------------------------------------------
            // --- Don't set these to nullptr because if one of the partners dies
            // --- before they migrate, then we need the pointers so we can remove
            // --- the relationship.
            // -------------------------------------------------------------------
            //male_partner = nullptr;
            //female_partner = nullptr;
            // -------------------------------------------------------------------

            SetParameters( nullptr );
        }
        else if( state != RelationshipState::MIGRATING )
        {
            std::ostringstream msg;
            msg << "Trying to migrate relationship id=" << GetSuid().data << " when current state=" << state ;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
    }

    bool
    Relationship::IsDiscordant()
    const
    {
        bool bPart1Infected = (male_partner && male_partner->IsInfected());
        bool bPart2Infected = (female_partner && female_partner->IsInfected());
        if( bPart1Infected != bPart2Infected )
        {
            return true;
        }
        return false;
    }

    float
    Relationship::GetTimer()
    const
    {
        return rel_timer;
    }

    float
    Relationship::GetDuration()
    const
    {
        return rel_duration;
    }

    float Relationship::GetStartTime() const {
        return start_time;
    }

    float Relationship::GetScheduledEndTime() const {
        return scheduled_end_time;
    }

    RelationshipType::Enum
    Relationship::GetType()
    const
    {
        return relationship_type;
    }

    suids::suid Relationship::GetPartnerId( const suids::suid& myID ) const
    {
        if( myID == GetMalePartnerId() )
        {
            return GetFemalePartnerId();
        }
        else if( myID == GetFemalePartnerId() )
        {
            return GetMalePartnerId();
        }
        else
        {
            std::stringstream ss;
            ss << "Unknown partner: rel-id=" << _suid.data << " male-id=" << GetMalePartnerId().data << " female-id=" << GetFemalePartnerId().data << " unknown-id=" << myID.data;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    suids::suid Relationship::GetMalePartnerId() const
    {
        return MALE_PARTNER_ID();
    }

    suids::suid Relationship::GetFemalePartnerId() const
    {
        return FEMALE_PARTNER_ID();
    }

    ExternalNodeId_t Relationship::GetOriginalNodeId() const
    {
        return original_node_id;
    }

    bool Relationship::IsMalePartnerAbsent() const
    {
        return (absent_male_partner_id != suids::nil_suid());
    }

    bool Relationship::IsFemalePartnerAbsent() const
    {
        return (absent_female_partner_id != suids::nil_suid());
    }

    float Relationship::GetCoitalRate() const
    {
        return p_rel_params->GetCoitalActRate();
    }

    ProbabilityNumber Relationship::getProbabilityUsingCondomThisAct() const
    {
        // TBD: In future we will get probs for both partners and use a configurable 
        // weighting but for now we'll just treat condoms as a male intervention in practice.
        //#define DECISION_WEIGHT (0.8f)
        //auto male_prob = male_partner->getProbabilityUsingCondomThisAct( GetType() );
        //auto female_prob = female_partner->getProbabilityUsingCondomThisAct( GetType() );
        //ProbabilityNumber prob = DECISION_WEIGHT * male_prob + (1.0f-DECISION_WEIGHT)*female_prob;

        ProbabilityNumber prob = 0.0;
        if( p_condom_usage != nullptr )
        {
            // ---------------------------------------------------------------------------------
            // --- if the relationship has its own condom usage specified, then let it override
            // --- that of the relationship type and any STIBarrier
            // ---------------------------------------------------------------------------------
            float year = male_partner->GetIndividualHuman()->GetParent()->GetTime().Year();
            prob = p_condom_usage->variableWidthAndHeightSigmoid( year );
        }
        else
        {
            // ---------------------------------------------------------------------------------
            // --- if the relationship does not have its own condom usage specified, then use any
            // --- STIBarrier specified, else use the parameters associated with the
            // --- relationship type.
            // ---------------------------------------------------------------------------------
            prob = male_partner->getProbabilityUsingCondomThisAct( p_rel_params );
        }

        return prob;
    }

    unsigned int Relationship::GetTotalCoitalActs() const
    {
        return total_coital_acts;
    }

    unsigned int Relationship::GetNumCoitalActs() const
    {
        return coital_acts_this_dt;
    }

    unsigned int Relationship::GetNumCoitalActsUsingCondoms() const
    {
        return coital_acts_this_dt_using_condoms;
    }

    bool Relationship::HasMigrated() const
    {
        return has_migrated;
    }

    suids::suid Relationship::GetMigrationDestination() const
    {
        return migration_destination;
    }

    REGISTER_SERIALIZABLE(Relationship);

    void Relationship::serialize(IArchive& ar, Relationship* obj)
    {
        Relationship& rel = *obj;

        if( ar.IsWriter() )
        {
            if( (rel.absent_male_partner_id == suids::nil_suid()) && (rel.male_partner != nullptr) )
            {
                rel.absent_male_partner_id = rel.male_partner->GetSuid();
            }
            if( (rel.absent_female_partner_id == suids::nil_suid()) && (rel.female_partner != nullptr) )
            {
                rel.absent_female_partner_id = rel.female_partner->GetSuid();
            }
        }

        ar.labelElement("_suid"                   ) & rel._suid;
        ar.labelElement("is_outside_pfa"          ) & rel.is_outside_pfa;
        ar.labelElement("state"                   ) & (uint32_t&)rel.state;
        ar.labelElement("previous_state"          ) & (uint32_t&)rel.previous_state;
        ar.labelElement("relationship_type"       ) & (uint32_t&)rel.relationship_type;
        ar.labelElement("termination_reason"      ) & (uint32_t&)rel.termination_reason;
        //p_rel_params don't serialize this
        //male_partner don't serialize this
        //female_partner don't serialize this
        ar.labelElement("absent_male_partner_id"            ) & rel.absent_male_partner_id;
        ar.labelElement("absent_female_partner_id"          ) & rel.absent_female_partner_id;
        ar.labelElement("rel_timer"                         ) & rel.rel_timer;
        ar.labelElement("rel_duration"                      ) & rel.rel_duration;
        ar.labelElement("start_time"                        ) & rel.start_time;
        ar.labelElement("scheduled_end_time"                ) & rel.scheduled_end_time;
        ar.labelElement("original_node_id"                  ) & (uint32_t&)rel.original_node_id;
        ar.labelElement("act_prob_vec"                      ) & rel.act_prob_vec;
        ar.labelElement("total_coital_acts"                 ) & rel.total_coital_acts;
        ar.labelElement("coital_acts_this_dt"               ) & rel.coital_acts_this_dt;
        ar.labelElement("coital_acts_this_dt_using_condoms" ) & rel.coital_acts_this_dt_using_condoms;
        ar.labelElement("has_migrated"                      ) & rel.has_migrated;
        ar.labelElement("migration_destination"             ) & rel.migration_destination;
        ar.labelElement("male_slot_number"                  ) & rel.male_slot_number;
        ar.labelElement("female_slot_number"                ) & rel.female_slot_number;

        if( ar.IsWriter() )
        {
            bool has_condom_usage = (rel.p_condom_usage != nullptr);
            ar.labelElement( "has_condom_usage" ) & has_condom_usage;
            if( has_condom_usage )
            {
                ar.labelElement( "condom_usage" ) & *rel.p_condom_usage;
            }
        }
        else
        {
            bool has_condom_usage = false;
            ar.labelElement( "has_condom_usage" ) & has_condom_usage;
            if( has_condom_usage )
            {
                rel.p_condom_usage = new Sigmoid();
                ar.labelElement( "condom_usage" ) & *rel.p_condom_usage;
            }

        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // SPECIFIC TYPES OF RELATIONSHIPS HERE //
    ///////////////////////////////////////////////////////////////////////////

    IRelationship* RelationshipFactory::CreateRelationship( RANDOMBASE* pRNG,
                                                            const suids::suid& rRelId,
                                                            IRelationshipParameters* pParams, 
                                                            IIndividualHumanSTI* male_partner, 
                                                            IIndividualHumanSTI* female_partner,
                                                            bool isOutsidePFA,
                                                            Sigmoid* pCondomUsage )
    {
        LOG_DEBUG_F( "(EEL) Creating %s %d between %s and %s.\n",
                    RelationshipType::pairs::lookup_key( pParams->GetType() ),
                    rRelId.data,
                    male_partner->toString().c_str(),
                    female_partner->toString().c_str()
                  );
        IRelationship* p_rel = new Relationship( pRNG, rRelId, pParams, male_partner, female_partner, isOutsidePFA, pCondomUsage );
        return p_rel ;
    }

}


