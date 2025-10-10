
#include "stdafx.h"

#include "IndividualMalaria.h"
#include "INodeContext.h"
#include "SusceptibilityMalaria.h"
#include "InfectionMalaria.h"
#include "Debug.h"
#include "Sigmoid.h"
#include "TransmissionGroupMembership.h"

#include "MalariaInterventionsContainer.h"
#include "SimulationConfig.h"
#include "MathFunctions.h"
#include "EventTrigger.h"
#include "Vector.h"
#include "ReportUtilitiesMalaria.h" // NASBADensityWithUncertainty() in MakeDiagnosticMeasurement()
#include "RANDOM.h"
#include "Exceptions.h"

SETUP_LOGGING( "IndividualMalaria" )

#ifndef WIN32
#define max(a,b) ( (a>b) ? a : b )
#endif

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(Malaria.Individual,IndividualHumanMalariaConfig)

    // ----------------- IndividualHumanMalariaConfig ---------------
    float                 IndividualHumanMalariaConfig::mean_sporozoites_per_bite         = 0.0f;
    float                 IndividualHumanMalariaConfig::base_sporozoite_survival_fraction = 0.0f;
    float                 IndividualHumanMalariaConfig::antibody_csp_killing_threshold    = 0.0f;
    float                 IndividualHumanMalariaConfig::antibody_csp_killing_invwidth     = 0.0f;
    std::vector<float>    IndividualHumanMalariaConfig::measurement_sensitivity;

    BEGIN_QUERY_INTERFACE_BODY(IndividualHumanMalariaConfig)
    END_QUERY_INTERFACE_BODY(IndividualHumanMalariaConfig)

    bool IndividualHumanMalariaConfig::Configure( const Configuration * config )
    {
        LOG_DEBUG( "Configure\n" );
        IndividualHumanConfig::enable_immunity = true;

        initConfigTypeMap( "Mean_Sporozoites_Per_Bite",          &mean_sporozoites_per_bite,         Mean_Sporozoites_Per_Bite_DESC_TEXT,          0.0f,  1000.0f, DEFAULT_SPOROZOITES_PER_BITE );
        initConfigTypeMap( "Base_Sporozoite_Survival_Fraction",  &base_sporozoite_survival_fraction, Base_Sporozoite_Survival_Fraction_DESC_TEXT,  0.0f,  1.0f,    DEFAULT_SPOROZOITE_SURVIVAL_FRACTION );
        initConfigTypeMap( "Antibody_CSP_Killing_Threshold",     &antibody_csp_killing_threshold,    Antibody_CSP_Killing_Threshold_DESC_TEXT,     1e-6f, 1e6f,    DEFAULT_ANTIBODY_CSP_KILLING_THRESHOLD );
        initConfigTypeMap( "Antibody_CSP_Killing_Inverse_Width", &antibody_csp_killing_invwidth,     Antibody_CSP_Killing_Inverse_Width_DESC_TEXT, 1e-6f, 1e6f,    DEFAULT_ANTIBODY_CSP_KILLING_INVWIDTH );

        float parasiteSmearSensitivity = 0.1f;
        float gametocyteSmearSensitivity = 0.1f;
        initConfigTypeMap( "Report_Parasite_Smear_Sensitivity",   &(parasiteSmearSensitivity),   Report_Parasite_Smear_Sensitivity_DESC_TEXT,   0.0001f, 100.0f, 0.1f );
        initConfigTypeMap( "Report_Gametocyte_Smear_Sensitivity", &(gametocyteSmearSensitivity), Report_Gametocyte_Smear_Sensitivity_DESC_TEXT, 0.0001f, 100.0f, 0.1f );

        bool ret = JsonConfigurable::Configure( config );
        if( ret && !JsonConfigurable::_dryrun )
        {
            measurement_sensitivity.push_back( parasiteSmearSensitivity   );
            measurement_sensitivity.push_back( gametocyteSmearSensitivity );
            measurement_sensitivity.push_back( 1.0 );
            measurement_sensitivity.push_back( 1.0 );
            measurement_sensitivity.push_back( 1.0 );
            measurement_sensitivity.push_back( 1.0 );
            measurement_sensitivity.push_back( 1.0 );
            release_assert( measurement_sensitivity.size() == MalariaDiagnosticType::pairs::count() );
        }
        return ret;
    }

    void IndividualHumanMalaria::InitializeStaticsMalaria( const Configuration * config )
    {
        SusceptibilityMalariaConfig immunity_config;
        immunity_config.Configure( config );
        InfectionMalariaConfig infection_config;
        infection_config.Configure( config );
        IndividualHumanMalariaConfig individual_config;
        individual_config.Configure( config );
    }

    // ----------------- IndividualHumanMalaria ---------------
    BEGIN_QUERY_INTERFACE_DERIVED(IndividualHumanMalaria, IndividualHumanVector)
        HANDLE_INTERFACE(IMalariaHumanContext)
        HANDLE_INTERFACE(IMalariaHumanInfectable)
    END_QUERY_INTERFACE_DERIVED(IndividualHumanMalaria, IndividualHumanVector)

    IndividualHumanMalaria::IndividualHumanMalaria(suids::suid _suid, double monte_carlo_weight, double initial_age, int gender)
    : IndividualHumanVector(_suid, monte_carlo_weight, initial_age, gender)
    , malaria_susceptibility(nullptr)
    , m_male_gametocytes(0)
    , m_female_gametocytes(0)
    , m_female_gametocytes_by_strain()
    , m_gametocytes_detected(0.0)
    //, m_clinical_symptoms_new()
    //, m_clinical_symptoms_continuing()
    , m_initial_infected_hepatocytes(0)
    , m_DiagnosticMeasurement()
    , m_CSP_antibody( nullptr )
    , m_MaxedInfDuration(0.0f)
    {
        ResetClinicalSymptoms();
        m_DiagnosticMeasurement.resize( MalariaDiagnosticType::pairs::count(), 0.0 );
    }

    IndividualHumanMalaria::IndividualHumanMalaria(INodeContext *context)
    : IndividualHumanVector(context)
    , malaria_susceptibility(nullptr)
    , m_male_gametocytes(0)
    , m_female_gametocytes(0)
    , m_female_gametocytes_by_strain()
    , m_gametocytes_detected(0.0)
    //, m_clinical_symptoms_new()
    //, m_clinical_symptoms_continuing()
    , m_initial_infected_hepatocytes(0)
    , m_DiagnosticMeasurement()
    , m_CSP_antibody( nullptr )
    , m_MaxedInfDuration( 0.0f )
    {
        ResetClinicalSymptoms();
        m_DiagnosticMeasurement.resize( MalariaDiagnosticType::pairs::count(), 0.0 );
    }

    IndividualHumanMalaria *IndividualHumanMalaria::CreateHuman(INodeContext *context, suids::suid id, double weight, double initial_age, int gender)
    {
        IndividualHumanMalaria *newhuman = _new_ IndividualHumanMalaria(id, weight, initial_age, gender);

        newhuman->SetContextTo(context);
        LOG_DEBUG_F( "Created human with age=%f\n", newhuman->m_age );

        return newhuman;
    }

    IndividualHumanMalaria::~IndividualHumanMalaria()
    {
        // deletion of infections handled by ~Individual
        // deletion of susceptibility handled by ~Individual
    }

    void IndividualHumanMalaria::PropagateContextToDependents()
    {
        IndividualHumanVector::PropagateContextToDependents();

        if( malaria_susceptibility == nullptr && susceptibility != nullptr)
        {
            if ( s_OK != susceptibility->QueryInterface(GET_IID(IMalariaSusceptibility), (void**)&malaria_susceptibility) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "susceptibility", "IMalariaSusceptibility", "Susceptibility" );
            }
        }
    }

    void IndividualHumanMalaria::setupInterventionsContainer()
    {
        vector_interventions = _new_ MalariaInterventionsContainer();
        interventions = vector_interventions;
    }

    void IndividualHumanMalaria::CreateSusceptibility(float imm_mod, float risk_mod)
    {
        SusceptibilityMalaria *newsusceptibility = SusceptibilityMalaria::CreateSusceptibility(dynamic_cast<IIndividualHumanContext*>(this), m_age, imm_mod, risk_mod);
        malaria_susceptibility = newsusceptibility;
        vector_susceptibility = newsusceptibility;
        susceptibility = newsusceptibility;

        // initialize CSP antibody pointer
        m_CSP_antibody = newsusceptibility->RegisterAntibody(MalariaAntibodyType::CSP, 0); // only type 0 for now
    }

    void IndividualHumanMalaria::setupMaternalAntibodies(IIndividualHumanContext* mother, INodeContext* node)
    {
        if ( mother )
        {
            IMalariaHumanContext* malaria_mother;
            if( s_OK != mother->QueryInterface(GET_IID(IMalariaHumanContext), (void**)&malaria_mother) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "mother", "IMalariaHumanContext", "IIndividualHumanContext" );
            }

            // If we are using individual pregnancies, initialize maternal antibodies dependent on mother's antibody history
            malaria_susceptibility->init_maternal_antibodies(malaria_mother->GetMalariaSusceptibilityContext()->get_fraction_of_variants_with_antibodies(MalariaAntibodyType::PfEMP1_major));
        }
        else
        {
            INodeMalaria* malaria_node;
            if( s_OK != node->QueryInterface(GET_IID(INodeMalaria), (void**)&malaria_node) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "node", "INodeMalaria", "INodeContext" );
            }

            // If we are doing births at population level, use the average antibody history of all possible mothers
            malaria_susceptibility->init_maternal_antibodies(malaria_node->GetMaternalAntibodyFraction());
        }
    }

    int IndividualHumanMalaria::GetInitialHepatocytes()
    {
        // If m_initial_infected_hepatocytes=0, this function is being called from initial infections at t=0 or an Outbreak intervention.
        // In that case, default to the mean number of infected hepatocytes.
        int initial_hepatocytes = int(IndividualHumanMalariaConfig::mean_sporozoites_per_bite * IndividualHumanMalariaConfig::base_sporozoite_survival_fraction);

        // A non-zero value for m_initial_infected_hepatocytes means
        // that the Poisson draw in ApplyTotalBitingExposure initiated the AcquireNewInfection function call.
        // (Or that a MalariaChallenge intervention has been called and ChallengeWithSporozoites returned true)
        if(m_initial_infected_hepatocytes > 0)
        {
            initial_hepatocytes = m_initial_infected_hepatocytes;
        }

        // Reset initial infected hepatocyte variable for next time steps
        m_initial_infected_hepatocytes = 0;

        return initial_hepatocytes;
    }

    IInfection* IndividualHumanMalaria::createInfection(suids::suid _suid)
    {
        int initial_hepatocytes = GetInitialHepatocytes();

        return InfectionMalaria::CreateInfection(dynamic_cast<IIndividualHumanContext*>(this), _suid, initial_hepatocytes);
    }


    void IndividualHumanMalaria::ExposeToInfectivity(float dt, TransmissionGroupMembership_t transmissionGroupMembership)
    {
        INodeVector* p_node_vector = nullptr;
        if ( s_OK != parent->QueryInterface(GET_IID(INodeVector), (void**)&p_node_vector) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "INodeVector", "INodeContext" );
        }

        // Here we track exposure in the antibody_capacity to CSP.  This is not used at the moment to reduce bite probability of success, but it could in the future as knowledge improves

        // Is there any exposure at all from mosquitoes?
        double infectivity = p_node_vector->GetTotalContagionGP( TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR  ).GetSum()
                           + p_node_vector->GetTotalContagionGP( TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_OUTDOOR ).GetSum();

        // If so, mark its presence and provide a slow increase in antibodies to CSP over time and exposure.
        // This is just a proxy for exposure with an approximately 3-year time constant.
        if ( infectivity > 0 )
        {
            m_CSP_antibody->UpdateAntibodyCapacityByRate( dt, infectivity * 0.001);  // 0.001 ~= 1 / (3*DAYSPERYEAR)
            m_CSP_antibody->SetAntigenicPresence(true);
        }
        else
        {
            m_CSP_antibody->SetAntigenicPresence(false);
        }

        // Now decide whether the individual's exposure results in an infection
        TransmissionGroupMembership_t dummy( -1 );
        IndividualHumanVector::ExposeToInfectivity( dt, dummy );
    }

    bool IndividualHumanMalaria::DidReceiveInfectiousBite()
    {
        // First calculate number of infectious bites
        // Downstream (in createInfection), we will use the number of infected hepatocytes to initialize new infections.
        bool was_bit = IndividualHumanVector::DidReceiveInfectiousBite();

        // Then do sporozoite challenge (caching inital infected hepatocyte count for createInfection)
        bool was_infected = was_bit && ChallengeWithBites( m_num_infectious_bites );

        return was_infected;
    }

    bool IndividualHumanMalaria::ChallengeWithBites( int n_infectious_bites )
    {
        int n_sporozoites = n_infectious_bites * IndividualHumanMalariaConfig::mean_sporozoites_per_bite;
        return ChallengeWithSporozoites( n_sporozoites );
    }

    bool IndividualHumanMalaria::ChallengeWithSporozoites( int n_sporozoites )
    {
        // TODO: make the survival probability some function of m_CSP_antibody->GetAntibodyConcentration();
        //       - compare to challenge studies with naive individuals (0) and with African adults (1)
        //       - compare to RTS,S phase IIa trial for no protection (NP), delayed patent parasitemia (DL), and protected (PR) population mean anti-CSP concentration
        //       - compare to RTS,S phase IIb and phase III trials for decay profile of anti-CSP concentration and protection for those with 20-2000 times higher antibody levels
        //       - verify the decay is not structurally problematic (need to specialize MalariaAntibodyCSP::UpdateAntibodyConcentration to deal with >1 concentrations)
        //       - verify the decay is scientifically accurate based on RTS,S trial numbers (again feed into if concentration>1 block of UpdateAntibodyConcentration)

        float sporozoite_survival_prob = IndividualHumanMalariaConfig::base_sporozoite_survival_fraction;

        float anti_csp_concentration = m_CSP_antibody->GetAntibodyConcentration();
        if ( anti_csp_concentration > 0 )
        {
            // TODO: is this an adequate functional form?
            sporozoite_survival_prob *= ( 1.0f - Sigmoid::variableWidthSigmoid( log10(anti_csp_concentration),
                                                                                log10(IndividualHumanMalariaConfig::antibody_csp_killing_threshold),
                                                                                IndividualHumanMalariaConfig::antibody_csp_killing_invwidth ) ); 
        }

        m_initial_infected_hepatocytes = GetRng()->Poisson( n_sporozoites * sporozoite_survival_prob );
        return ( m_initial_infected_hepatocytes > 0 ) ? true : false;
    }

    void IndividualHumanMalaria::Update( float currenttime, float dt )
    {
        if( HasMaxInfections() )
        {
            m_MaxedInfDuration += dt;
        }

        IndividualHumanVector::Update( currenttime, dt );

        // ---------------------------------------------------------
        // --- Make diagnostic measurements to be used by reports
        // --- We make them once so all reports use the same values
        // ---------------------------------------------------------
        CalculateDiagnosticMeasurementsForReports();

        if( !HasMaxInfections() )
        {
            m_MaxedInfDuration = 0.0;
        }
    }

    void IndividualHumanMalaria::ReportInfectionState()
    {
        IndividualHumanVector::ReportInfectionState();
        broadcaster->TriggerObservers( GetEventContext(), EventTrigger::NewMalariaInfectionObject );
    }

    void IndividualHumanMalaria::UpdateInfectiousness(float dt)
    {
        UpdateGametocyteCounts(dt);

        // infectiousness calculated based on total gametocytes
        infectiousness = CalculateInfectiousness();

        // contagion deposited by outcrossing of strains proportional to relative concentrations
        DepositInfectiousnessFromGametocytes();
    }

    void IndividualHumanMalaria::UpdateGametocyteCounts(float dt)
    {
        m_male_gametocytes = 0;
        m_female_gametocytes = 0;
        m_female_gametocytes_by_strain.clear();

        if( dt == 0 )
        {
            // during initialization, this method can be called with dt = 0
            return;
        }

        // Check for mature gametocyte drug killing
        IMalariaDrugEffects* imde = nullptr;
        if (s_OK != GetInterventionsContext()->QueryInterface(GET_IID(IMalariaDrugEffects), (void **)&imde))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "GetInterventionsContext()", "IMalariaDrugEffects", "IIndividualHumanInterventionsContext" );
        }

        // Loop over infections and add newly matured male and female gametocytes by strain
        for (auto infection : infections)
        {
            // Cast from Infection --> InfectionMalaria
            IInfectionMalaria *tempinf = nullptr;
            if( infection->QueryInterface( GET_IID(IInfectionMalaria), (void**)&tempinf ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "Infection", "IInfectionMalaria", "tempinf" );
            }

            // Get mature gametocytes at this time step
            // N.B. malariaCycleGametocytes is called once per asexual cycle (several days),
            int64_t tmp_male_gametocytes   = tempinf->get_MaleGametocytes(   GametocyteStages::Mature );
            int64_t tmp_female_gametocytes = tempinf->get_FemaleGametocytes( GametocyteStages::Mature );

            // Decay half-life--Sinden, R. E., G. A. Butcher, et al. (1996). "Regulation of Infectivity of Plasmodium to the Mosquito Vector." Advances in Parasitology 38: 53-117.
            // Smalley, M. E. and R. E. Sinden (1977). "Plasmodium falciparum gametocytes: their longevity and infectivity." Parasitology 74(01): 1-8.
            const IStrainIdentity& r_strain_id = infection->GetInfectiousStrainID();
            float drugGametocyteKill = imde->get_drug_gametocyteM( r_strain_id );
            double pkill = EXPCDF( -dt * (0.277 + drugGametocyteKill) ); // half-life of 2.5 days corresponds to a decay time constant of 3.6 days, 0.277 = 1/3.6

            // apply kill probability to mature gametocytes, including effect of gametocytocidal drugs
            tempinf->apply_MatureGametocyteKillProbability( pkill );

            // No mature gametocytes
            if ( (tmp_male_gametocytes == 0) && (tmp_female_gametocytes == 0) ) continue;

            StoreGametocyteCounts( r_strain_id, tmp_female_gametocytes, tmp_male_gametocytes );

            m_female_gametocytes += tmp_female_gametocytes;
            m_male_gametocytes += tmp_male_gametocytes;
        }
    }

    void IndividualHumanMalaria::StoreGametocyteCounts( const IStrainIdentity& rStrain,
                                                        int64_t femaleMatureGametocytes,
                                                        int64_t maleMatureGametocytes )
    {
        // !!! DanB - I don't think the comment below applies anymore because this map is cleared each time step !!!
        // Add new gametocytes to those carried over (and not killed) from the previous time steps

        if( femaleMatureGametocytes > 0 )
        {
            StrainIdentity tmp_strainIDs;
            tmp_strainIDs.SetAntigenID( rStrain.GetAntigenID() );
            tmp_strainIDs.SetGeneticID( rStrain.GetGeneticID() );
            m_female_gametocytes_by_strain[ tmp_strainIDs ] += femaleMatureGametocytes;
        }
    }

    float IndividualHumanMalaria::CalculateInfectiousness() const
    {
        float tmp_infectiousness = 0.0;
        if( m_female_gametocytes > 0 )
        {
            release_assert( malaria_susceptibility );
            float inv_microliters_blood = malaria_susceptibility->get_inv_microliters_blood();

            // Now add a factor to limit inactivation of gametocytes by inflammatory cytokines
            // Important for slope of infectivity v. gametocyte counts
            double fever_effect = malaria_susceptibility->get_cytokines();
            fever_effect = Sigmoid::basic_sigmoid( SusceptibilityMalariaConfig::cytokine_gametocyte_inactivation, float( fever_effect ) );
            // fever_effect*=0.95;

            // Infectivity is reviewed by Sinden, R. E., G. A. Butcher, et al. (1996). "Regulation of Infectivity of Plasmodium to the Mosquito Vector." Advances in Parasitology 38: 53-117.
            // model based on data from Jeffery, G. M. and D. E. Eyles (1955). "Infectivity to Mosquitoes of Plasmodium Falciparum as Related to Gametocyte Density and Duration of Infection." Am J Trop Med Hyg 4(5): 781-789.
            // and Schneider, P., J. T. Bousema, et al. (2007). "Submicroscopic Plasmodium falciparum gametocyte densities frequently result in mosquito infection." Am J Trop Med Hyg 76(3): 470-474.
            // 2 due to bloodmeal and other factor due to conservative estimate for macrogametocyte ookinete transition, can be a higher reduction due to immune response
            // that factor also includes effect of successful fertilization with male gametocytes
            tmp_infectiousness = float( EXPCDF( -double( m_female_gametocytes ) 
                                                * inv_microliters_blood * MICROLITERS_PER_BLOODMEAL
                                                * SusceptibilityMalariaConfig::base_gametocyte_mosquito_survival
                                                * (1.0 - fever_effect) ) ); //temp function, see vector_parameter_scratch.xlsx

            LOG_DEBUG_F( "Gametocytes: %lld (male) %lld (female).  Infectiousness=%0.2g\n", m_male_gametocytes, m_female_gametocytes, infectiousness );

            // Effects of transmission-reducing immunity.  N.B. interventions on vector success are not here, since they depend on vector-population-specific behavior
            float modtransmit = susceptibility->getModTransmit() * interventions->GetInterventionReducedTransmit();
            tmp_infectiousness *= modtransmit;
        }
        return tmp_infectiousness;
    }

    float IndividualHumanMalaria::GetWeightedInfectiousness()
    {
        // Host weight is the product of MC weighting and relative biting
        float host_vector_weight = float(GetMonteCarloWeight() * GetRelativeBitingRate());
        float weighted_infectiousnesss = host_vector_weight * infectiousness;

        return weighted_infectiousnesss;
    }

    uint32_t FromOutcrossing( RANDOMBASE* pRNG, uint32_t id1, uint32_t id2 )
    {
        uint32_t mask = pRNG->ul();

        uint32_t child_strain = (id1 & mask) | (id2 & ~mask);
        LOG_DEBUG_F( "%d + %d with mask=%d --> %d\n", id1, id2, mask, child_strain );
        return child_strain;
    }

    void IndividualHumanMalaria::DepositInfectiousnessFromGametocytes()
    {
        float weighted_infectiousnesss = GetWeightedInfectiousness();

        // Effects from vector intervention container
        IVectorInterventionsEffects* ivie = GetVectorInterventionEffects();

        // Here we deposit human-to-vector infectiousness based on proportional outcrossing of strain IDs
        gametocytes_strain_map_t::const_iterator gc1,gc2,end=m_female_gametocytes_by_strain.end();
        for (gc1=m_female_gametocytes_by_strain.begin(); gc1!=end; ++gc1)
        {
            for (gc2=gc1; gc2!=end; ++gc2)
            {
                // Fractional weight is product of component weights
                float strain_weight;
                if (gc1==gc2)
                {
                    strain_weight = pow( float(gc1->second) / float(m_female_gametocytes), 2 );
                    DepositFractionalContagionByStrain( weighted_infectiousnesss * strain_weight, ivie, gc1->first.GetAntigenID(), gc1->first.GetGeneticID() );
                    continue;
                }

                // Two off-diagonal contributions given how pairwise iteration is done in this loop
                strain_weight = 2.0f * gc1->second * gc2->second / pow( float(m_female_gametocytes), 2 );
                LOG_DEBUG_F("Crossing two strains with weight %0.2f and %0.2f\n", gc1->second/float(m_female_gametocytes), gc2->second/(float)m_female_gametocytes);

                // Genetic ID from first component
                int geneticID = gc1->first.GetGeneticID();
                if ( geneticID != gc2->first.GetGeneticID() ) 
                {
                    // One outcrossing realization if genetic IDs are different
                    geneticID = FromOutcrossing( GetRng(), geneticID, gc2->first.GetGeneticID() );
                    LOG_DEBUG_F("Crossing geneticID %d + %d --> %d\n", gc1->first.GetGeneticID(), gc2->first.GetGeneticID(), geneticID);
                }

                // Deposit fractional infectiousness to each of indoor and outdoor pools
                if ( gc1->first.GetAntigenID() == gc2->first.GetAntigenID() )
                {
                    DepositFractionalContagionByStrain( weighted_infectiousnesss * strain_weight, ivie, gc1->first.GetAntigenID(), geneticID );
                }
                else
                {
                    // Deposit half the weight to each if antigenIDs are different.  
                    // Note that geneticID is not outcrossed independently for the different antigen IDs.
                    DepositFractionalContagionByStrain( 0.5f * weighted_infectiousnesss * strain_weight, ivie, gc1->first.GetAntigenID(), geneticID );
                    DepositFractionalContagionByStrain( 0.5f * weighted_infectiousnesss * strain_weight, ivie, gc2->first.GetAntigenID(), geneticID );
                    LOG_DEBUG_F("Depositing contagion with antigenIDs %d and %d for geneticID=%d\n", gc1->first.GetAntigenID(), gc2->first.GetAntigenID(), geneticID);
                }
            }
        }
    }

    void IndividualHumanMalaria::DepositFractionalContagionByStrain(float weight, IVectorInterventionsEffects* ivie, float antigenID, float geneticID)
    {
        INodeVector* p_node_vector = nullptr;
        if ( s_OK != parent->QueryInterface(GET_IID(INodeVector), (void**)&p_node_vector) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "INodeVector", "INodeContext" );
        }

        StrainIdentity id;
        id.SetAntigenID( antigenID );
        id.SetGeneticID( geneticID );
        p_node_vector->DepositFromIndividual( id, ivie->GetblockIndoorVectorTransmit() *weight, TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_INDOOR );
        p_node_vector->DepositFromIndividual( id, ivie->GetblockOutdoorVectorTransmit()*weight, TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_OUTDOOR );
    }

    void IndividualHumanMalaria::ResetClinicalSymptoms()
    {
        ZERO_ARRAY( m_clinical_symptoms_new );
        ZERO_ARRAY( m_clinical_symptoms_continuing );
    }
 
    void IndividualHumanMalaria::ClearNewInfectionState()
    {
         IndividualHumanVector::ClearNewInfectionState();
         ResetClinicalSymptoms();
    }

    void IndividualHumanMalaria::AddClinicalSymptom( ClinicalSymptomsEnum::Enum symptom, bool isNew )
    {
        // ---------------------------------------------------------------------------------------------
        // --- "Or" the isNew status because we just care that it become new during this time step.
        // --- m_clinicanl_systems_XXX gets cleared once per time step, but this method could be called
        // --- multiple times due to the infectious update loop being done multiple times per time step.
        // ---------------------------------------------------------------------------------------------
        m_clinical_symptoms_new[ symptom ] |= isNew;
        m_clinical_symptoms_continuing[ symptom ] = true;

        if( isNew )
        {
            // Trigger observers of new clinical and severe malaria episodes
            if( symptom == ClinicalSymptomsEnum::CLINICAL_DISEASE )
            {
                broadcaster->TriggerObservers( GetEventContext(), EventTrigger::NewClinicalCase );
            }
            else if( symptom == ClinicalSymptomsEnum::SEVERE_DISEASE )
            {
                broadcaster->TriggerObservers( GetEventContext(), EventTrigger::NewSevereCase );
            }
        }
    }

    bool IndividualHumanMalaria::HasClinicalSymptomNew(ClinicalSymptomsEnum::Enum symptom) const
    {
        return m_clinical_symptoms_new[symptom];
    }

    bool IndividualHumanMalaria::HasClinicalSymptomContinuing( ClinicalSymptomsEnum::Enum symptom ) const
    {
        return m_clinical_symptoms_continuing[ symptom ];
    }

    IMalariaSusceptibility* IndividualHumanMalaria::GetMalariaSusceptibilityContext() const
    {
        return malaria_susceptibility;
    }

    std::vector< std::pair<int,int> > IndividualHumanMalaria::GetInfectingStrainIds() const
    {
        std::vector< std::pair<int,int> > strainIds;
        for (auto inf : infections)
        {
            const IStrainIdentity& r_strain_id = inf->GetInfectiousStrainID();
            strainIds.push_back( std::make_pair( r_strain_id.GetAntigenID(), r_strain_id.GetGeneticID() ) );
        }
        return strainIds;
    }

    float IndividualHumanMalaria::GetParasiteDensity() const
    {
        return malaria_susceptibility->get_parasite_density();
    }

    float IndividualHumanMalaria::GetGametocyteDensity() const
    {
        release_assert( malaria_susceptibility != nullptr );
        return (m_female_gametocytes + m_male_gametocytes) * malaria_susceptibility->get_inv_microliters_blood();
    }

    float IndividualHumanMalaria::MakeDiagnosticMeasurement( MalariaDiagnosticType::Enum mdType, float measurementSensitivity )
    {
        float measurement = 0.0;
        if( measurementSensitivity == 0.0 )
        {
            return measurement;
        }

        float true_parasite_density = malaria_susceptibility->get_parasite_density();
        float true_gametocyte_density = GetGametocyteDensity();
        switch( mdType )
        {
            case MalariaDiagnosticType::BLOOD_SMEAR_PARASITES:
                // perform a typical blood smear (default
                // take .1 microliters of blood and count parasites
                // 10xPoisson distributed with mean .1xparasite_density
                measurement = float( 1.0 / measurementSensitivity * GetRng()->Poisson( measurementSensitivity * true_parasite_density ) ); // parasites / microliter
                break;

            case MalariaDiagnosticType::BLOOD_SMEAR_GAMETOCYTES:
                measurement = float( 1.0 / measurementSensitivity * GetRng()->Poisson( measurementSensitivity * true_gametocyte_density ) ); // gametocytes / microliter
                break;

            case MalariaDiagnosticType::PCR_PARASITES:
                measurement = ReportUtilitiesMalaria::NASBADensityWithUncertainty( GetRng(), true_parasite_density );
                break;

            case MalariaDiagnosticType::PCR_GAMETOCYTES:
                measurement = ReportUtilitiesMalaria::NASBADensityWithUncertainty( GetRng(), true_gametocyte_density );
                break;

            case MalariaDiagnosticType::PF_HRP2:
                measurement = malaria_susceptibility->GetPfHRP2();
                break;

            case MalariaDiagnosticType::TRUE_PARASITE_DENSITY:
                measurement = true_parasite_density;
                break;

            case MalariaDiagnosticType::FEVER:
                measurement = malaria_susceptibility->get_fever();
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "mdType", mdType,
                                                         MalariaDiagnosticType::pairs::lookup_key( mdType ) );
        }
        return measurement;
    }


    void IndividualHumanMalaria::CalculateDiagnosticMeasurementsForReports()
    {
        // ----------------------------------------------------------------
        // --- We calculate the diagnostic measurements all at once so that
        // --- all reports will use the same values.  We don't require this
        // --- for the interventions.  They calculate a new value on the fly
        // --- because that diagnostic could have different sensitivities.
        // ----------------------------------------------------------------
        m_DiagnosticMeasurement.clear();
        for( int i = 0; i < MalariaDiagnosticType::pairs::count(); ++i )
        {
            MalariaDiagnosticType::Enum md_type = MalariaDiagnosticType::Enum( i );
            float measurement = MakeDiagnosticMeasurement( md_type, IndividualHumanMalariaConfig::measurement_sensitivity[ i ] );
            m_DiagnosticMeasurement.push_back( measurement );
        }
    }

    float IndividualHumanMalaria::GetDiagnosticMeasurementForReports( MalariaDiagnosticType::Enum mdType ) const
    {
        return m_DiagnosticMeasurement[ mdType ];
    }

    const SimulationConfig*
    IndividualHumanMalaria::params()
    const
    {
        return GET_CONFIGURABLE(SimulationConfig);
    }

    void IndividualHumanMalaria::SetContextTo(INodeContext* context)
    {
        IndividualHumanVector::SetContextTo(context);

        // We could be called from CreateHuman in which case we haven't yet initialized malaria_susceptibility.
        if (malaria_susceptibility != nullptr)
        {
            m_CSP_antibody = malaria_susceptibility->RegisterAntibody(MalariaAntibodyType::CSP, 0);
        }
    }

    bool IndividualHumanMalaria::HasMaxInfections() const
    {
        return (this->infections.size() == IndividualHumanConfig::max_ind_inf);
    }

    float IndividualHumanMalaria::GetMaxInfectionDuration() const
    {
        return m_MaxedInfDuration;
    }

    REGISTER_SERIALIZABLE(IndividualHumanMalaria);

    void serialize(IArchive& ar, IndividualHumanMalaria::gametocytes_strain_map_t& mapping)
    {
        size_t count = ar.IsWriter() ? mapping.size() : -1;
        ar.startArray(count);
        if (ar.IsWriter())
        {
            for (auto& entry : mapping)
            {
                StrainIdentity* strain = const_cast<StrainIdentity*>(&entry.first);
                ar.startObject();
                    ar.labelElement("key"); StrainIdentity::serialize(ar, *strain);
                    ar.labelElement("value") & entry.second;
                ar.endObject();
            }
        }
        else
        {
            for (size_t i = 0; i < count; i++)
            {
                StrainIdentity strain;
                int64_t value;
                ar.startObject();
                    ar.labelElement("key"); StrainIdentity::serialize(ar, strain);
                    ar.labelElement("value") & value;
                ar.endObject();
                mapping[strain] = value;
            }
        }
        ar.endArray();
    }

    void IndividualHumanMalaria::serialize(IArchive& ar, IndividualHumanMalaria* obj)
    {
        IndividualHumanVector::serialize(ar, obj);
        IndividualHumanMalaria& individual = *obj;
        ar.labelElement( "m_male_gametocytes"             ) & individual.m_male_gametocytes;
        ar.labelElement( "m_female_gametocytes"           ) & individual.m_female_gametocytes;
        ar.labelElement( "m_female_gametocytes_by_strain" ); Kernel::serialize(ar, individual.m_female_gametocytes_by_strain);
        ar.labelElement( "m_gametocytes_detected"         ) & individual.m_gametocytes_detected;
        ar.labelElement( "m_clinical_symptoms_new"        ); ar.serialize( individual.m_clinical_symptoms_new, ClinicalSymptomsEnum::CLINICAL_SYMPTOMS_COUNT );
        ar.labelElement( "m_clinical_symptoms_continuing" ); ar.serialize( individual.m_clinical_symptoms_continuing, ClinicalSymptomsEnum::CLINICAL_SYMPTOMS_COUNT );
        ar.labelElement( "m_initial_infected_hepatocytes" ) & individual.m_initial_infected_hepatocytes;
        ar.labelElement( "m_DiagnosticMeasurement"        ) & individual.m_DiagnosticMeasurement;

        // ----------------------------------------------------------------------
        // --- This is a pointer to an object held in the Susceptibility object. 
        // --- It will be re-set after de-serialization. See SetContextTo()
        // ----------------------------------------------------------------------
        //ar.labelElement("m_CSP_antibody") & individual.m_CSP_antibody;
        // ----------------------------------------------------------------------
    }
}

