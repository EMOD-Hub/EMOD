
#include "stdafx.h"

#include "IndividualMalariaGenetics.h"
#include "MalariaGeneticsContexts.h"
#include "INodeContext.h"
#include "ParasiteCohort.h"
#include "VectorInterventionsContainer.h"
#include "Susceptibility.h"
#include "SusceptibilityMalaria.h"
#include "NodeEventContext.h"
#include "ParasiteGenetics.h"
#include "InfectionMalariaGenetics.h"
#include "StrainIdentityMalariaGenetics.h"

SETUP_LOGGING( "IndividualMalariaGenetics" )

namespace Kernel
{
    // ----------------- IndividualHumanMalariaGenetics ---------------
    BEGIN_QUERY_INTERFACE_DERIVED( IndividualHumanMalariaGenetics, IndividualHumanMalaria )
    END_QUERY_INTERFACE_DERIVED( IndividualHumanMalariaGenetics, IndividualHumanMalaria )

    void IndividualHumanMalariaGenetics::InitializeStaticsMalariaGenetics( const Configuration * config )
    {
        SusceptibilityMalariaConfig immunity_config;
        immunity_config.Configure( config );
        InfectionMalariaGeneticsConfig infection_config;
        infection_config.Configure( config );
        IndividualHumanMalariaConfig individual_config;
        individual_config.Configure( config );
    }



    IndividualHumanMalariaGenetics *IndividualHumanMalariaGenetics::CreateHuman( INodeContext *context, suids::suid id, double weight, double initial_age, int gender )
    {
        IndividualHumanMalariaGenetics *newhuman = _new_ IndividualHumanMalariaGenetics( id, weight, initial_age, gender );

        newhuman->SetContextTo( context );
        LOG_DEBUG_F( "Created human id %d with age=%f\n", newhuman->GetSuid().data , newhuman->m_age);

        return newhuman;
    }

    IndividualHumanMalariaGenetics::IndividualHumanMalariaGenetics( suids::suid _suid, double monte_carlo_weight, double initial_age, int gender )
        : IndividualHumanMalaria( _suid, monte_carlo_weight, initial_age, gender )
        , m_pNodeGenetics( nullptr )
        , m_MatureGametocytesFemale()
        , m_MatureGametocytesMale()
    {
    }

    IndividualHumanMalariaGenetics::IndividualHumanMalariaGenetics( INodeContext *context )
        : IndividualHumanMalaria( context )
        , m_pNodeGenetics( nullptr )
        , m_MatureGametocytesFemale()
        , m_MatureGametocytesMale()
    {
    }

    IndividualHumanMalariaGenetics::~IndividualHumanMalariaGenetics()
    {
        // need to make sure we clear these when a person dies so that the
        // ParasiteCohorts are removed.
        ClearMatureGametocyteCohorts();
    }

    void IndividualHumanMalariaGenetics::SetContextTo( INodeContext* context )
    {
        IndividualHumanMalaria::SetContextTo( context );

        // It can be null when emigrating.
        if( context != nullptr )
        {
            if( context->QueryInterface( GET_IID( INodeMalariaGenetics ), (void**)&m_pNodeGenetics ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "m_context", "INodeMalariaGenetics", "INodeContext" );
            }
        }
    }

    IInfection* IndividualHumanMalariaGenetics::createInfection( suids::suid _suid )
    {
        int initial_hepatocytes = GetInitialHepatocytes();

        return InfectionMalariaGenetics::CreateInfection( static_cast<IIndividualHumanContext*>(this), _suid, initial_hepatocytes );
    }

    void IndividualHumanMalariaGenetics::UpdateInfectiousness( float dt )
    {
        ClearMatureGametocyteCohorts();

        UpdateGametocyteCounts( dt );

        // infectiousness calculated based on total gametocytes
        infectiousness = CalculateInfectiousness();

        IVectorInterventionsEffects* p_ivie = GetVectorInterventionEffects();

        GeneticProbability prob_bitten_indoor  = (1.0 - p_ivie->GetHostNotAvailable()       ) * GetMonteCarloWeight() * GetRelativeBitingRate();
        GeneticProbability prob_bitten_outdoor = (1.0 - p_ivie->GetOutdoorHostNotAvailable()) * GetMonteCarloWeight() * GetRelativeBitingRate();

        // the one-to-one transmission idea of Depositing Contagion
        m_pNodeGenetics->AddPerson( GetSuid().data,
                                    prob_bitten_indoor,
                                    prob_bitten_outdoor,
                                    p_ivie,
                                    infectiousness,
                                    malaria_susceptibility->get_inv_microliters_blood(),
                                    &m_MatureGametocytesFemale,
                                    &m_MatureGametocytesMale );
    }

    void IndividualHumanMalariaGenetics::StoreGametocyteCounts( const IStrainIdentity& rStrain,
                                                                int64_t femaleMatureGametocytes,
                                                                int64_t maleMatureGametocytes )
    {
        IParasiteIdGenerator* p_id_gen = m_pNodeGenetics->GetParasiteIdGenerator();

        if( femaleMatureGametocytes > 0 )
        {
            ParasiteCohort* p_pc_female = new ParasiteCohort( p_id_gen->GetNextParasiteSuid().data,
                                                              ParasiteState::GAMETOCYTE_FEMALE,
                                                              &rStrain,
                                                              0,
                                                              femaleMatureGametocytes );
            const StrainIdentityMalariaGenetics* p_si_genetics_const = static_cast<const StrainIdentityMalariaGenetics*>(&(p_pc_female->GetStrainIdentity()));
            StrainIdentityMalariaGenetics* p_si_genetics = const_cast<StrainIdentityMalariaGenetics*>(p_si_genetics_const);
            const InfectionSourceInfo& r_sporozoite_info = p_si_genetics->GetSporozoiteInfo();
            p_si_genetics->SetFemaleGametocyteInfo( InfectionSourceInfo( 0.0,
                                                                         0,
                                                                         0,
                                                                         0,
                                                                         GetSuid().data,
                                                                         r_sporozoite_info.GetInfectionID(),
                                                                         p_si_genetics->GetGenome().GetID() ) );
            p_si_genetics->SetSporozoiteInfo( InfectionSourceInfo() ); //clear it out so can be used for these gametocytes becoming sporozoites
            m_MatureGametocytesFemale.push_back( p_pc_female );
        }

        if( maleMatureGametocytes > 0 )
        {
            ParasiteCohort* p_pc_male   = new ParasiteCohort( p_id_gen->GetNextParasiteSuid().data,
                                                              ParasiteState::GAMETOCYTE_MALE,
                                                              &rStrain,
                                                              0,
                                                              maleMatureGametocytes   );
            const StrainIdentityMalariaGenetics* p_si_genetics_const = static_cast<const StrainIdentityMalariaGenetics*>(&(p_pc_male->GetStrainIdentity()));
            StrainIdentityMalariaGenetics* p_si_genetics = const_cast<StrainIdentityMalariaGenetics*>(p_si_genetics_const);
            const InfectionSourceInfo& r_sporozoite_info = p_si_genetics->GetSporozoiteInfo();
            p_si_genetics->SetMaleGametocyteInfo( InfectionSourceInfo( 0.0,
                                                                       0,
                                                                       0,
                                                                       0,
                                                                       GetSuid().data,
                                                                       r_sporozoite_info.GetInfectionID(),
                                                                       p_si_genetics->GetGenome().GetID() ) );
            p_si_genetics->SetSporozoiteInfo( InfectionSourceInfo() ); //clear it out so can be used for these gametocytes becoming sporozoites
            m_MatureGametocytesMale.push_back( p_pc_male );
        }
    }

    void IndividualHumanMalariaGenetics::ClearMatureGametocyteCohorts()
    {
        for( auto p_parasite_cohort : m_MatureGametocytesFemale )
        {
            delete p_parasite_cohort;
        }
        m_MatureGametocytesFemale.clear();
        for( auto p_parasite_cohort : m_MatureGametocytesMale )
        {
            delete p_parasite_cohort;
        }
        m_MatureGametocytesMale.clear();
    }

    void IndividualHumanMalariaGenetics::ExposeToInfectivity( float dt, TransmissionGroupMembership_t tgm )
    {
#if 0
See IndvidualHumanMalaria::ExposeToInfectivity() about CSP
        if( infectivity > 0 )
        {
            m_CSP_antibody->UpdateAntibodyCapacityByRate( dt, infectivity * 0.001 );  // 0.001 ~= 1 / (3*DAYSPERYEAR)
            m_CSP_antibody->SetAntigenicPresence( true );
        }
        else
        {
            m_CSP_antibody->SetAntigenicPresence( false );
        }
#endif

        uint32_t num_bites_received = 0;
        const std::vector<IParasiteCohort*>& r_sporozoites = m_pNodeGenetics->GetSporozoitesFromBites( GetSuid().data, num_bites_received );
        if( r_sporozoites.size() == 0 )
        {
            return;
        }
        m_num_infectious_bites = num_bites_received;

        IIndividualEventBroadcaster* broadcaster = GetNodeEventContext()->GetIndividualEventBroadcaster();
        broadcaster->TriggerObservers( GetEventContext(), EventTrigger::ReceivedInfectiousBites );

        // the number of sporozoites going into the infection are set
        // in ChallengeWithSporozoites() and retrieved in createInfection().
        if( ParasiteGenetics::GetInstance()->IsFPGSimulatingBaseModel() )
        {
            int n_sporozoites = m_num_infectious_bites * IndividualHumanMalariaConfig::mean_sporozoites_per_bite;
            if( ChallengeWithSporozoites( n_sporozoites ) )
            {
                const IStrainIdentity* p_strain = &(*r_sporozoites.begin())->GetStrainIdentity();
                AcquireNewInfection( p_strain );
                broadcaster->TriggerObservers( GetEventContext(), EventTrigger::VectorToHumanTransmission );
            }
        }
        else
        {
            for( auto p_ipc : r_sporozoites )
            {
                if( ChallengeWithSporozoites( p_ipc->GetPopulation() ) )
                {
                    IInfection* p_prev_inf = m_pNewInfection;
                    AcquireNewInfection( &(p_ipc->GetStrainIdentity()) );
                    if ((m_pNewInfection != nullptr) && (p_prev_inf != m_pNewInfection))
                    { 
                        const StrainIdentityMalariaGenetics& r_si_genetics = static_cast<const StrainIdentityMalariaGenetics&>(m_pNewInfection->GetInfectiousStrainID());
                        uint32_t bite_id = r_si_genetics.GetSporozoiteBiteID();
                        uint32_t inf_id = m_pNewInfection->GetSuid().data;
                        uint32_t person_id = GetSuid().data;
                        LOG_VALID_F("Person %d got bite %d creating infection %d\n", person_id, bite_id, inf_id); 
                        broadcaster->TriggerObservers( GetEventContext(), EventTrigger::VectorToHumanTransmission );
                    }
                }
            }
        }
    }

    REGISTER_SERIALIZABLE( IndividualHumanMalariaGenetics );

    void IndividualHumanMalariaGenetics::serialize( IArchive& ar, IndividualHumanMalariaGenetics* obj )
    {
        IndividualHumanMalaria::serialize( ar, obj );
        IndividualHumanMalariaGenetics& individual = *obj;

        // These are sort of temporary and don't need to be serialized
        // m_pNodeGenetics
        // m_MatureGametocytesFemale
        // m_MatureGametocytesMale
    }
}

