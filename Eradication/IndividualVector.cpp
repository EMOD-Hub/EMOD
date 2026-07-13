
#include "stdafx.h"

#include "IndividualVector.h"
#include "SusceptibilityVector.h"
#include "InfectionVector.h"
#include "ITransmissionGroups.h"
#include "Common.h"
#include "Debug.h"
#include "IContagionPopulation.h"
#include "ContagionPopulationSimple.h"
#include "TransmissionGroupMembership.h"
#include "INodeContext.h"
#include "VectorInterventionsContainer.h"
#include "RANDOM.h"
#include "SimulationConfig.h"
#include "IContagionPopulationGP.h"
#include "NodeEventContext.h"

#include "Log.h"

SETUP_LOGGING( "IndividualVector" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(IndividualHumanVector, IndividualHuman)
        HANDLE_INTERFACE(IIndividualHumanVectorContext)
    END_QUERY_INTERFACE_DERIVED(IndividualHumanVector, IndividualHuman)

    IndividualHumanVector::IndividualHumanVector(suids::suid _suid, double monte_carlo_weight, double initial_age, int gender)
        : Kernel::IndividualHuman(_suid, float(monte_carlo_weight), float(initial_age), gender)
        , m_strain_exposure()
        , m_total_exposure(0.0f)
        , m_num_infectious_bites(0)
        , vector_susceptibility(nullptr)
        , vector_interventions(nullptr)
    {
    }

    IndividualHumanVector::IndividualHumanVector(INodeContext *context)
        : Kernel::IndividualHuman(context)
        , m_strain_exposure()
        , m_total_exposure(0.0f)
        , m_num_infectious_bites(0)
        , vector_susceptibility(nullptr)
        , vector_interventions(nullptr)
    {
    }

    IndividualHumanVector *IndividualHumanVector::CreateHuman(INodeContext *context, suids::suid id, double MCweight, double init_age, int gender)
    {
        Kernel::IndividualHumanVector *newhuman = _new_ Kernel::IndividualHumanVector(id, MCweight, init_age, gender);

        newhuman->SetContextTo(context);
        LOG_DEBUG_F( "Created human with age=%f\n", newhuman->m_age );

        return newhuman;
    }

    IndividualHumanVector::~IndividualHumanVector()
    {
        // deletion of individuals handled by parent destructor
        // deletion of susceptibility handled by parent destructor
    }

    void IndividualHumanVector::InitializeStaticsVector( const Configuration * config )
    {
        SusceptibilityVectorConfig immunity_config;
        immunity_config.Configure( config );
    }

    void IndividualHumanVector::InitializeHuman()
    {
        // -----------------------------------------------------------------------------
        // --- We need to ensure that the probabilities in VectorInterventionsContainer
        // --- get initialized correctly.  If we don't do this, the math gets messed up
        // --- in VectorPopulation.
        // -----------------------------------------------------------------------------
        float dt = GET_CONFIGURABLE( SimulationConfig )->Sim_Tstep;
        vector_interventions->InfectiousLoopUpdate( dt );
        vector_interventions->Update( dt );
    }

    void IndividualHumanVector::PropagateContextToDependents()
    {
        IndividualHuman::PropagateContextToDependents();

        if(susceptibility && !vector_susceptibility)
        {
            if ( s_OK != susceptibility->QueryInterface(GET_IID(IVectorSusceptibilityContext), (void**)&vector_susceptibility) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "susceptibility", "IVectorSusceptibilityContext", "Susceptibility" );
            }
        }

        if(interventions && !vector_interventions)
        {
            vector_interventions = static_cast<VectorInterventionsContainer*>(interventions);
        }
    }

    void IndividualHumanVector::setupInterventionsContainer()
    {
        vector_interventions = _new_ VectorInterventionsContainer();
        interventions = vector_interventions; // initialize base class pointer to same object
    }

    void IndividualHumanVector::CreateSusceptibility(float imm_mod, float risk_mod)
    {
        SusceptibilityVector *newsusceptibility = SusceptibilityVector::CreateSusceptibility(this, imm_mod, risk_mod);
        vector_susceptibility = newsusceptibility;
        susceptibility = newsusceptibility; // initialize base class pointer to same object

        susceptibility->SetContextTo(this);
    }


    void IndividualHumanVector::ExposeToInfectivity(float dt, TransmissionGroupMembership_t transmissionGroupMembership)
    {
        // Reset counters
        m_strain_exposure.clear();
        m_total_exposure = 0;

        // Expose individual to all pools in weighted collection (i.e. indoor + outdoor)
        LOG_VALID("Exposure to contagion: vector to human.\n");
        parent->ExposeIndividual( this, transmissionGroupMembership, dt );

        // Decide based on total exposure to infectious bites
        // whether the individual becomes infected and with what strain
        if ( m_total_exposure > 0 )
        {
            ApplyTotalBitingExposure();
        }
    }

    void IndividualHumanVector::UpdateGroupPopulation(float size_changes)
    {
        // Update nodepool population for both human-vector and vector-human since we use the same normalization for both
        float host_vector_weight = float(GetMonteCarloWeight() * GetRelativeBitingRate());
        IPKeyValueContainer not_used_by_NodeVector;
        parent->UpdateTransmissionGroupPopulation( not_used_by_NodeVector, size_changes, host_vector_weight );
        LOG_DEBUG_F("updated population for both human and vector, with size change %f and monte carlo weight %f.\n", size_changes, host_vector_weight);
    }

    bool IndividualHumanVector::DidReceiveInfectiousBite()
    {
        // We don't do the draw on probability of *any* infectious bites, i.e. EXPCDF(-m_total_exposure)
        // We will instead do a Poisson draw on how many infectious bites, with the equivalent behavior for zero bites.

        m_num_infectious_bites = GetRng()->Poisson( m_total_exposure );
        if( m_num_infectious_bites > 0 )
        {
            IIndividualEventBroadcaster* broadcaster = GetNodeEventContext()->GetIndividualEventBroadcaster();
            broadcaster->TriggerObservers( GetEventContext(), EventTrigger::ReceivedInfectiousBites );
        }
        return (m_num_infectious_bites > 0);
    }

    void IndividualHumanVector::ApplyTotalBitingExposure()
    {
        if( DidReceiveInfectiousBite() )
        {
            // Choose a strain based on a weighted draw over values from all vector-to-human pools
            float strain_cdf_draw = GetRng()->e() * m_total_exposure;
            std::vector<strain_exposure_t>::iterator it = std::lower_bound( m_strain_exposure.begin(), m_strain_exposure.end(), strain_cdf_draw, compare_strain_exposure_float_less());
            LOG_DEBUG_F( "Mosquito->Human infection transmission based on total exposure %f. Existing infections = %d.\n", m_total_exposure, GetInfections().size() );
            AcquireNewInfection( &(it->first) );
        }
    }

    void IndividualHumanVector::Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route )
    {
        release_assert( cp );
        release_assert( susceptibility );
        release_assert( interventions );
        release_assert( vector_interventions );

        IContagionPopulationGP* cp_gp = nullptr;
        if ( s_OK != (const_cast<IContagionPopulation*>(cp))->QueryInterface(GET_IID(IContagionPopulationGP), (void**)&cp_gp) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "cp", "IContagionPopulationGP", "IContagionPopulation" );
        }

        float acqmod = GetRelativeBitingRate() * susceptibility->getModAcquire() * interventions->GetInterventionReducedAcquire();

        GeneticProbability gp_acqmod = 1.0f;
        switch( transmission_route )
        {
            case TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR:
                gp_acqmod = vector_interventions->GetblockIndoorVectorAcquire();
                break;

            case TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_OUTDOOR:
                gp_acqmod = vector_interventions->GetblockOutdoorVectorAcquire();
                break;
        
            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "transmission_route", transmission_route, TransmissionRoute::pairs::lookup_key( transmission_route ) );
        }

        // -----------------------------------------------------------------------------------
        // --- Accumulate vector of pairs of strain ids and cumulative infection probability
        // --- We have a GeneticProbability because we taking into account the genome of the
        // --- vectors that were depositing and any resistance in this person's interventions.
        // -----------------------------------------------------------------------------------
        GeneticProbability total_contagion = cp_gp->GetTotalContagionGP();
        GeneticProbability gp_total = total_contagion * gp_acqmod;
        float infection_probability = gp_total.GetSum() * acqmod * dt;
        if ( infection_probability > 0 )
        {
            // With a weighted random draw, pick a strain from the ContagionPopulation CDF
            StrainIdentity strain_id;
            if( cp->ResolveInfectingStrain(&strain_id) )
            {
                // Increment total exposure
                m_total_exposure += infection_probability;

                AddExposure( strain_id, m_total_exposure, transmission_route );
            }
        }
    }

    void IndividualHumanVector::AddExposure( const StrainIdentity& rStrainId,
                                             float totalExposure,
                                             TransmissionRoute::Enum transmission_route )
    {
        // Push this exposure and strain back to the storage array for all vector-to-human pools (e.g. indoor, outdoor)
        m_strain_exposure.push_back( std::make_pair( rStrainId, totalExposure ) );
    }

    void IndividualHumanVector::UpdateInfectiousness(float dt)
    {
        infectiousness = 0;

        typedef std::map< StrainIdentity, float >     strain_infectivity_map_t;
        typedef strain_infectivity_map_t::value_type  strain_infectivity_t;
        static strain_infectivity_map_t infectivity_by_strain;
        infectivity_by_strain.clear();

        // Loop once over all infections, caching strains and infectivity.
        // If total infectiousness exceeds unity, we will normalize all strains down accordingly.
        for (auto infection : infections)
        {
            release_assert( infection );
            float tmp_infectiousness = infection->GetInfectiousness();
            infectiousness += tmp_infectiousness;

            if ( tmp_infectiousness > 0 )
            {
                const IStrainIdentity& r_strain_id = infection->GetInfectiousStrainID();
                StrainIdentity tmp_strainIDs;
                tmp_strainIDs.SetAntigenID( r_strain_id.GetAntigenID() );
                tmp_strainIDs.SetGeneticID( r_strain_id.GetGeneticID() );
                infectivity_by_strain[tmp_strainIDs] += tmp_infectiousness;
            }
        }

        if( infectiousness == 0 )
        {
            return;
        }

        // Effects of transmission-reducing immunity.  N.B. interventions on vector success are not here, since they depend on vector-population-specific behavior
        release_assert( susceptibility );
        release_assert( interventions );
        float modtransmit = susceptibility->getModTransmit() * interventions->GetInterventionReducedTransmit();

        // Maximum individual infectiousness is set here, capping the sum of unmodified infectivity at prob=1
        float truncate_infectious_mod = (infectiousness > 1 ) ? 1.0f/infectiousness : 1.0f;
        infectiousness *= truncate_infectious_mod * modtransmit;

        // Host weight is the product of MC weighting and relative biting
        float host_vector_weight = float(GetMonteCarloWeight() * GetRelativeBitingRate());

        // Effects from vector intervention container
        IVectorInterventionsEffects* ivie = GetVectorInterventionEffects();

        INodeVector* p_node_vector = nullptr;
        if ( s_OK != parent->QueryInterface(GET_IID(INodeVector), (void**)&p_node_vector) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "INodeVector", "INodeContext" );
        }

        // Loop again over infection strains, depositing (downscaled) infectivity modified by vector intervention effects etc. (indoor + outdoor)
        GeneticProbability indoor_contagion  = ivie->GetblockIndoorVectorTransmit()  * (host_vector_weight * truncate_infectious_mod * modtransmit);
        GeneticProbability outdoor_contagion = ivie->GetblockOutdoorVectorTransmit() * (host_vector_weight * truncate_infectious_mod * modtransmit);
        for (auto& infectivity : infectivity_by_strain)
        {
            const StrainIdentity *id = &(infectivity.first);
            GeneticProbability inf_indoor_contagion  = indoor_contagion  * infectivity.second;
            GeneticProbability inf_outdoor_contagion = outdoor_contagion * infectivity.second;
            LOG_DEBUG_F( "Depositing contagion from human to vector (indoor & outdoor) with biting-rate-driven weight of %f and combined modifiers of %f.\n",
                         host_vector_weight,
                         indoor_contagion.GetSum()
                       );
            p_node_vector->DepositFromIndividual( *id, inf_indoor_contagion,  TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_INDOOR );
            p_node_vector->DepositFromIndividual( *id, inf_outdoor_contagion, TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_OUTDOOR );
        }
    }

    IInfection *IndividualHumanVector::createInfection(suids::suid _suid)
    {
        return InfectionVector::CreateInfection(this, _suid);
    }

    float IndividualHumanVector::GetRelativeBitingRate(void) const
    {
        // I don't think we need this but it was needed by Dengue at one point
        if( vector_susceptibility == nullptr && susceptibility != nullptr)
        {
            if ( s_OK != susceptibility->QueryInterface(GET_IID(IVectorSusceptibilityContext), (void**)&vector_susceptibility) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                               "susceptibility", "IVectorSusceptibilityContext", "Susceptibility" );
            }
        }
        release_assert( vector_susceptibility );
        return vector_susceptibility->GetRelativeBitingRate();
    }

    int IndividualHumanVector::GetNumInfectiousBites() const
    {
        return m_num_infectious_bites;
    }

    IVectorInterventionsEffects* IndividualHumanVector::GetVectorInterventionEffects() const
    {
        return vector_interventions;
    }

    void IndividualHumanVector::ReportInfectionState()
    {
        LOG_DEBUG( "ReportInfectionState\n" );
        // Is infection reported this turn?
        // Will only implement delayed reporting (for fever response) later
        // 50% reporting immediately
        if( GetRng()->e() < .5 )
        {
            m_new_infection_state = NewInfectionState::NewAndDetected;
        }
        else
        {
            release_assert( parent );
            m_new_infection_state = NewInfectionState::NewInfection;
        }
    }

    REGISTER_SERIALIZABLE(IndividualHumanVector);

    void IndividualHumanVector::serialize(IArchive& ar, IndividualHumanVector* obj)
    {
        IndividualHumanVector& individual = *obj;

        IndividualHuman::serialize(ar, obj);
        ar.labelElement("m_strain_exposure");
            Kernel::serialize(ar, individual.m_strain_exposure);
        ar.labelElement("m_total_exposure") & individual.m_total_exposure;
    }

    IArchive& serialize(IArchive& ar, std::vector<strain_exposure_t>& vec)
    {
        size_t count = ar.IsWriter() ? vec.size() : 0xDEADBEEF;
        ar.startArray(count);
        if (!ar.IsWriter())
        {
            vec.resize(count);
        }

        for (auto& entry : vec)
        {
            ar.startObject();
            ar.labelElement("strain"); StrainIdentity::serialize(ar, entry.first);
            ar.labelElement("weight") & entry.second;
            ar.endObject();
        }
        ar.endArray();

        return ar;
    }
}
