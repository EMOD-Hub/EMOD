
#include "stdafx.h"
#include "VectorSurveillanceEventCoordinator.h"
#include "SimulationEventContext.h"
#include "InterventionFactory.h"
#include "ReportStatsByIP.h"
#include "SimulationConfig.h"
#include "VectorSpeciesParameters.h"
#include "VectorParameters.h"
#include "IVectorCohort.h"
#include "VectorPopulation.h"
#include "IdmDateTime.h"
#include "DistributionFactory.h"
#include "PythonSupport.h"

SETUP_LOGGING("VectorSurveillanceEventCoordinator")

#define ENABLE_PYTHON_RESPONDER 1


namespace Kernel
{    

    // ------------------------------------------------------------------------
    // --- VectorCounter
    // ------------------------------------------------------------------------

    IMPL_QUERY_INTERFACE1( VectorCounter, IConfigurable )

    VectorCounter::VectorCounter()
        : JsonConfigurable()
        , m_pSampleSizeDistribution(nullptr)
        , m_CounterType(VectorCounterType::ALLELE_FREQ)
        , m_Species("")
        , m_Gender(VectorGender::VECTOR_FEMALE)
        , m_SampleSize(0)
        , m_UpdatePeriod(30.0)
        , m_NumVectorsSampled(0)
        , m_Names()
        , m_Fractions()
        , m_IsDoneCounting(true)
        , m_NumTimeStepsCounted(0)
        , m_PossibleGenomeList()
        , m_PossibleGenomeMap()
    {
    }

    VectorCounter::~VectorCounter()
    {
        delete m_pSampleSizeDistribution;
        for( auto p_gcn : m_PossibleGenomeList )
        {
            delete p_gcn;
        }
    }

    bool VectorCounter::Configure(const Configuration* inputJson)
    {
        if( GET_CONFIGURABLE(SimulationConfig) != nullptr )
        {
            VectorParameters* p_vp = GET_CONFIGURABLE(SimulationConfig)->vector_params;
            const jsonConfigurable::tDynamicStringSet& species_names = p_vp->vector_species.GetSpeciesNames();
            m_Species.constraint_param = &species_names;
        }
        m_Species.constraints = "Vector_Species_Params";
        initConfigTypeMap("Species", &m_Species, VC_Species_DESC_TEXT);

        initConfig("Gender", m_Gender, inputJson,
            MetadataDescriptor::Enum("Gender", VC_Gender_DESC_TEXT, MDD_ENUM_ARGS(VectorGender)));

        initConfigTypeMap("Update_Period", &m_UpdatePeriod, VC_Update_Period_DESC_TEXT, 0, 999999, 30.0f);

        initConfig("Count_Type", m_CounterType, inputJson, 
            MetadataDescriptor::Enum("Count_Type", VC_Count_Type_DESC_TEXT, MDD_ENUM_ARGS(VectorCounterType)));

        DistributionFunction::Enum sample_size_function(DistributionFunction::CONSTANT_DISTRIBUTION);
        initConfig("Sample_Size_Distribution", sample_size_function, inputJson, MetadataDescriptor::Enum("Sample_Size_Distribution", VC_Sample_Size_Distribution_DESC_TEXT, MDD_ENUM_ARGS(DistributionFunction)));
        m_pSampleSizeDistribution = DistributionFactory::CreateDistribution(this, sample_size_function, "Sample_Size", inputJson);

        bool is_configured = JsonConfigurable::Configure(inputJson);
        if( is_configured && !JsonConfigurable::_dryrun )
        {
        }
        return is_configured;
    }

    bool VectorCounter::HasBeenConfigured() const
    {
        // ------------------------------------------------------------------
        // --- Need to call from owner incase the nameof the counter is wrong
        // ------------------------------------------------------------------
        return ( m_pSampleSizeDistribution != nullptr );
    }

    VectorGender::Enum VectorCounter::GetGender() const
    {
        return m_Gender;
    }

    uint32_t VectorCounter:: GetSampleSize() const
    {
        return m_SampleSize;
    }

    float VectorCounter::GetUpdatePeriod() const
    {
        return m_UpdatePeriod;
    }

    VectorCounterType::Enum VectorCounter::GetCounterType() const
    {
        return m_CounterType;
    }

    void VectorCounter::SetCounterType( VectorCounterType::Enum countertype )
    {
        m_CounterType = countertype;
    }

    jsonConfigurable::ConstrainedString VectorCounter::GetSpecies() const 
    {
        return m_Species;
    }

    std::set<uint32_t> VectorCounter::DetermineVectorsToSample( const std::vector<INodeEventContext*>& rCachedNodes )
    {
        release_assert(GET_CONFIGURABLE(SimulationConfig) != nullptr); // if sim not running, we shouldn't even be here

        uint32_t total_vectors = 0;
        for( auto cachedNode : rCachedNodes )
        {
            INodeContext* pNC = cachedNode->GetNodeContext();
   
            INodeVector* pnv = NULL;
            if( s_OK != pNC->QueryInterface(GET_IID(INodeVector), (void**)&pnv) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNC", "INodeVector", "INodeContext" );
            }

            const VectorPopulationReportingList_t& r_vprl = pnv->GetVectorPopulationReporting();
            for( auto p_vpr : r_vprl )
            {
                if( p_vpr->get_SpeciesID() == GetSpecies() )
                {
                    if( m_Gender == VectorGender::VECTOR_BOTH_GENDERS || m_Gender == VectorGender::VECTOR_FEMALE )
                    {
                        total_vectors += p_vpr->getCount( VectorStateEnum::STATE_ADULT      );
                        total_vectors += p_vpr->getCount( VectorStateEnum::STATE_INFECTED   );
                        total_vectors += p_vpr->getCount( VectorStateEnum::STATE_INFECTIOUS );
                    }
                    if( (m_Gender == VectorGender::VECTOR_BOTH_GENDERS) || (m_Gender == VectorGender::VECTOR_MALE) )
                    {
                        total_vectors += p_vpr->getCount( VectorStateEnum::STATE_MALE );
                    }
                }
            }
        }

        std::set<uint32_t> selected_indicies;
        if (total_vectors == 0) { return selected_indicies; } // if no vectors, return emtpy indicies set

        // ----------------------------------------------------------------------------------
        // --- Robert Floyd's Algorithm for Sampling without Replacement
        // --- http://www.nowherenearithaca.com/2013/05/robert-floyds-tiny-and-beautiful.html
        // ----------------------------------------------------------------------------------
        RANDOMBASE* pRNG = rCachedNodes[0]->GetRng(); // getting the randomstream for first node

        uint32_t N = total_vectors;
        uint32_t M = VectorCounter::m_pSampleSizeDistribution->Calculate( pRNG );
        if (N <= M) // in case our total vector population is smaller than sample size
        {
            LOG_WARN_F("Available population for sampling (%u) is smaller than the sample size (%u), using the entire population. \n", N, M);
            for (uint32_t i = 0; i < N; i++) {
                selected_indicies.insert(i);
            }
        }
        else
        {
            selected_indicies = pRNG->chooseMofN(M, N);
        }
        return selected_indicies;
    }


    // purposelly making rSelectedIndicies non-const so that we can remove values
    // once we find a vector for the indicie
    std::vector<IVectorCohort*> VectorCounter::SelectVectors( const std::vector<INodeEventContext*>& rCachedNodes,
                                                              std::set<uint32_t>& rSelectedIndicies )
    {      
        bool first_cohort = true;
        std::vector<IVectorCohort*> chosen_vectors_cohorts;
        if (rSelectedIndicies.size() == 0) // if there's no indicies selected (means population = 0), return empty set
        {
            return chosen_vectors_cohorts;
        }

        // we keep track of "vector indicies" (vectors aren't actually indexed) within the cohort by incrementing 
        // the min index and max index within the cohort using the cohort's population
        uint32_t current_index_min_max[2] = { 0, 0 };
        vector_cohort_visit_function_t func =
            [ &chosen_vectors_cohorts, &current_index_min_max, &rSelectedIndicies, &first_cohort ]( IVectorCohort* cohort )
        {
            uint32_t cohort_pop = cohort->GetPopulation();
            if (first_cohort) // special case
            {
                current_index_min_max[1] = cohort_pop - 1;
                first_cohort = false;
            }
            else
            {
                current_index_min_max[0] = current_index_min_max[1] + 1;
                current_index_min_max[1] = current_index_min_max[1] + cohort_pop;
            }
            std::vector<uint32_t> found_indicies = {};
            for (auto index : rSelectedIndicies)
            {
                //if index is less than min -> release assert 'cause something went wrong with logic
                release_assert(index >= current_index_min_max[0]);
                if (index <= current_index_min_max[1]) 
                {
                    chosen_vectors_cohorts.push_back(cohort);
                    found_indicies.push_back(index);
                }
                else //means no more indicies within this cohort (indicies are sorted, because std::set)
                {
                    break;
                }
                
            }
            //clean up selected_indicies so we don't check found ones again
            for (auto index : found_indicies) 
            {
                rSelectedIndicies.erase(index);
            }
        };

        for( auto cachedNode : rCachedNodes )
        {
            INodeContext* pNC = cachedNode->GetNodeContext();
            INodeVector* pnv = NULL;
            if( s_OK != pNC->QueryInterface(GET_IID(INodeVector), (void**)&pnv) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNC", "INodeVector", "INodeContext" );
            }

            const VectorPopulationReportingList_t& r_vprl = pnv->GetVectorPopulationReporting();
            for( auto p_vpr : r_vprl )
            {
                if( p_vpr->get_SpeciesID() == GetSpecies() )
                {
                    p_vpr->visitVectors( func, m_Gender );
                }
            }
        }
        return chosen_vectors_cohorts;
    }


    void VectorCounter::CalculateAlleleFrequencies( const std::vector<INodeEventContext*>& rCachedNodes,
                                                    const std::vector<IVectorCohort*>& rChosenVectors )
    {
        // getting gene lookup map
        VectorSpeciesCollection& r_species = GET_CONFIGURABLE(SimulationConfig)->vector_params->vector_species;
        const VectorGeneCollection& genes = r_species.GetSpecies(m_Species).genes;

        // ---------------------------------------------------------------------------------
        // --- Populate m_Names such that there is one "combo" for each allele.
        // --- This will allow for m_Fractions to have one value for each allele.
        // ---------------------------------------------------------------------------------
        for( int locus_index = 0; locus_index < genes.Size();  ++locus_index )
        {
            const VectorGene* p_gene = genes[ locus_index ];
            for( int allele_index = 0; allele_index < p_gene->GetNumAllele(); ++allele_index )
            {
                const VectorAllele* p_allele =  p_gene->GetAllele( allele_index );
                if( p_allele != nullptr )
                {
                    m_Names.push_back( p_allele->GetName() );
                    m_Fractions.push_back( 0.0f );
                }
            }
        }

        if (rChosenVectors.size() == 0) // if there's no vectors sampled, return without trying to calculate anything
        {
            return;
        }
        // ---------------------------------------------------------------------------
        // --- For each vector, count the alleles in the genome.
        // --- We want vectors in the outer loop so that the vector and its genome get
        // --- loaded into memory once per vector.
        // ---------------------------------------------------------------------------
        for( auto p_cohort : rChosenVectors )
        {
            int combo_index = 0;
            for( int locus_index = 0; locus_index < genes.Size();  ++locus_index )
            {
                std::pair<uint8_t, uint8_t> allele_indexes_in_genome = p_cohort->GetGenome().GetLocus( locus_index );

                const VectorGene* p_gene = genes[ locus_index ];
                for( int allele_index = 0; allele_index < p_gene->GetNumAllele(); ++allele_index )
                {
                    const VectorAllele* p_allele =  p_gene->GetAllele( allele_index );
                    if( p_allele != nullptr )
                    {
                        if( allele_indexes_in_genome.first == allele_index )
                        {
                            m_Fractions[ combo_index ] += 1.0f;
                        }
                        if( allele_indexes_in_genome.second == allele_index )
                        {
                            m_Fractions[ combo_index ] += 1.0f;
                        }
                        ++combo_index;
                    }
                }
            }
        }

        // --------------------------------------------------------------------------------------------
        // --- Previously, m_NamesFractions just contained number found of each allele.
        // --- Dividing by the total possible number of alleles should turn this into allele frequency.
        // --------------------------------------------------------------------------------------------
        int num_alleles_sampled = rChosenVectors.size() * 2; // x2 to account for 2 alleles per locus
        for( int i = 0; i < m_Fractions.size(); ++i )
        {
            m_Fractions[ i ] /= float( num_alleles_sampled );
        }
    }

    void VectorCounter::CreatePossibleGenomeMap( INodeEventContext* rCachedNode )
    {
        INodeVector * pNodeVector = NULL;
        if (s_OK != rCachedNode->GetNodeContext()->QueryInterface(GET_IID(INodeVector), (void**) &pNodeVector) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pNC", "INodeVector", "INodeContext");
        }

        const VectorPopulationReportingList_t& vector_pop_list = pNodeVector->GetVectorPopulationReporting();
        release_assert( vector_pop_list.size() > 0 );

        IVectorPopulationReporting* p_ivpr = nullptr;
        for (auto vp : vector_pop_list)
        {
            if (m_Species == vp->get_SpeciesID())
            {
                p_ivpr = vp;
            }
        }
        release_assert( p_ivpr != nullptr );

        std::vector<PossibleGenome> pg_list = p_ivpr->FindPossibleGenomes( m_Gender, true );
        release_assert( pg_list.size() > 0 );

        for( auto& r_pg : pg_list )
        {
            GenomeCountName* p_gcn = new GenomeCountName( r_pg.genome, 0, r_pg.name );
            m_PossibleGenomeList.push_back( p_gcn );
            m_PossibleGenomeMap[ p_gcn->genome.GetBits() ] = p_gcn;

            for( auto& r_sim_genome : r_pg.similar_genomes )
            {
                m_PossibleGenomeMap[ r_sim_genome.GetBits() ] = p_gcn;
            }
        }
    }

    void VectorCounter::CalculateGenomeFractions( const std::vector<INodeEventContext*>& rCachedNodes,
                                                  const std::vector<IVectorCohort*>& rChosenVectors )
    {
        release_assert( rCachedNodes.size() > 0 );

        if( m_PossibleGenomeMap.size() == 0 )
        {
            CreatePossibleGenomeMap( rCachedNodes[0] );
        }

        if (rChosenVectors.size() == 0)  // if we're not looking at any vectors create arrays with all 0s for fractions and return
        {
            for (auto p_gcn : m_PossibleGenomeList)
            {
                m_Names.push_back(p_gcn->name);
                m_Fractions.push_back(0.0f);
            }
            return;
        }

        for( auto p_cohort : rChosenVectors )
        {
            VectorGameteBitPair_t bits = p_cohort->GetGenome().GetBits();
            m_PossibleGenomeMap[ bits ]->count += 1;
        }

        // -----------------------------------------------------------------
        // --- Convert the map into the arrays of genome names and fractions
        // -----------------------------------------------------------------
        m_NumVectorsSampled = rChosenVectors.size();
        for( auto p_gcn : m_PossibleGenomeList )
        {
            m_Names.push_back( p_gcn->name );
            m_Fractions.push_back( float( p_gcn->count ) / float( m_NumVectorsSampled ) );
        }
    }

    void VectorCounter::CollectStatistics( const std::vector<INodeEventContext*>& rCachedNodes ) 
    {
        std::set<uint32_t> selected_indicies = DetermineVectorsToSample( rCachedNodes );

        std::vector<IVectorCohort*> chosen_vectors_cohorts = SelectVectors( rCachedNodes, selected_indicies );

        // setting this to pass to responder
        m_NumVectorsSampled = chosen_vectors_cohorts.size();
        m_Names.clear();
        m_Fractions.clear();
        for( auto p_gcn : m_PossibleGenomeList )
        {
            p_gcn->count = 0;
        }

        switch( m_CounterType )
        {
            case VectorCounterType::ALLELE_FREQ:
                CalculateAlleleFrequencies( rCachedNodes, chosen_vectors_cohorts );
                break;

            case VectorCounterType::GENOME_FRACTION:
                CalculateGenomeFractions( rCachedNodes, chosen_vectors_cohorts );
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__,
                                                         "m_CounterType", m_CounterType, VectorCounterType::pairs::lookup_key( m_CounterType ) );
        }
    }

    void VectorCounter::StartCounting()
    {
        ResetCounting();

        // ----------------------------------------------------------------------------
        // --- Set this to the update period so that the first time we go into Update()
        // --- we say we are done counting.  This will cause the vectors to be sampled
        // --- when this coordinators is first told to start.
        // ----------------------------------------------------------------------------
        m_NumTimeStepsCounted = m_UpdatePeriod;
    }

    void VectorCounter::ResetCounting()
    {
        m_NumTimeStepsCounted = 0;
        m_IsDoneCounting = false;

        m_NumVectorsSampled = 0;
        m_Names.clear();
        m_Fractions.clear();
    }

    bool VectorCounter::IsDoneCounting() const
    {
        return m_IsDoneCounting;
    }

    void VectorCounter::Update(float dt)
    {
        ++m_NumTimeStepsCounted;
        if (m_NumTimeStepsCounted >= m_UpdatePeriod)
        {
            m_IsDoneCounting = true;
        }
    }

    uint32_t VectorCounter::GetNumVectorsSampled() const
    {
        return m_NumVectorsSampled;
    }

    const std::vector<std::string>& VectorCounter::GetNames() const
    {
        return m_Names;
    }

    const std::vector<float>& VectorCounter::GetFractions() const
    {
        return m_Fractions;
    }
 
    // ------------------------------------------------------------------------
    // --- VectorResponder
    // ------------------------------------------------------------------------

    IMPL_QUERY_INTERFACE1( VectorResponder, IConfigurable )

#define PYTHON_FUNC_NAME_CREATE  "create_responder"
#define PYTHON_FUNC_NAME_DELETE  "delete_responder"
#define PYTHON_FUNC_NAME_RESPOND "respond"

 static suids::distributed_generator* p_RESPONDER_ID_GENERATOR = nullptr;

    VectorResponder::VectorResponder()
        : JsonConfigurable()
        , m_ResponderID(-1)
        , m_CoordinatorName()
        , m_SurveyCompletedEvent()
    {
        if( p_RESPONDER_ID_GENERATOR == nullptr )
        {
            p_RESPONDER_ID_GENERATOR = new suids::distributed_generator( EnvPtr->MPI.Rank, EnvPtr->MPI.NumTasks );
        }
        m_ResponderID = (*p_RESPONDER_ID_GENERATOR)().data;
    }

    VectorResponder::~VectorResponder()
    {
#ifdef ENABLE_PYTHON_RESPONDER
        // Call into python script to notify of new individual
        PyObject* pFunc = static_cast<PyObject*>(PythonSupport::GetPyFunction( PythonSupport::SCRIPT_VECTOR_SURVEILLANCE,
                                                                               PYTHON_FUNC_NAME_DELETE ));
        if( pFunc )
        {
            PyObject* vars   = Py_BuildValue( "ls", m_ResponderID, m_CoordinatorName.c_str() );
            PyObject* retVal = PyObject_CallObject( pFunc, vars );

            Py_XDECREF(vars);
            Py_XDECREF(retVal);
        }
#endif
    }

    bool VectorResponder::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Survey_Completed_Event", &m_SurveyCompletedEvent, VSEC_Survey_Completed_Event_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( inputJson );
        return ret;
    }

    void VectorResponder::SetContextTo( ISimulationEventContext* sim, IEventCoordinatorEventContext* isec )
    {
        m_CoordinatorName = isec->GetName();
        m_pSim            = sim;
        m_Parent          = isec;

#ifdef ENABLE_PYTHON_RESPONDER
        // Call into python script to notify of new individual 
        PyObject* pFunc = static_cast<PyObject*>(Kernel::PythonSupport::GetPyFunction( Kernel::PythonSupport::SCRIPT_VECTOR_SURVEILLANCE,
                                                                                       PYTHON_FUNC_NAME_CREATE ));
        if( pFunc )
        {
            // pass individual id
            PyObject* vars   = Py_BuildValue( "ls", m_ResponderID, m_CoordinatorName.c_str() );
            PyObject* retVal = PyObject_CallObject( pFunc, vars );

            Py_XDECREF(vars);
            Py_XDECREF(retVal);
        }
#endif

    }

    bool VectorResponder::Respond( uint32_t numVectorsSampled,
                                   const std::vector<std::string>& rNames,
                                   const std::vector<float> rFractions )
    {
        release_assert( rNames.size() == rFractions.size() );

        float time = m_pSim->GetSimulationTime().time;

        std::vector<std::string> event_names;
#ifdef ENABLE_PYTHON_RESPONDER

        static PyObject* pFunc = nullptr;
        if( pFunc == nullptr )
        {
            pFunc = static_cast<PyObject*>( PythonSupport::GetPyFunction( PythonSupport::SCRIPT_VECTOR_SURVEILLANCE,
                                                                          PYTHON_FUNC_NAME_RESPOND ) );
        }
        if( pFunc == nullptr )
        {
            PyErr_Print();
            ostringstream msg;
            msg << "Failed to get handle to python function '" << PYTHON_FUNC_NAME_RESPOND << "'. ";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        PyObject* py_allele_combos = PyList_New( rNames.size() );
        PyObject* py_fractions     = PyList_New( rNames.size() );
        for( int i = 0; i < rNames.size(); ++i )
        {
            // SetItem "steals" the reference to the items that are set - we don't need to decrement counter
            PyList_SetItem( py_allele_combos, i, PyUnicode_FromString( rNames[ i ].c_str() ) );
            PyList_SetItem( py_fractions,     i, PyFloat_FromDouble(   rFractions[ i ]     ) );
        }

        // PyTuple_SetItem "steals the reference - we don't need to decrement counters
        PyObject* vars = PyTuple_New(6);
        PyTuple_SetItem( vars, 0, PyFloat_FromDouble(   time                      ) );
        PyTuple_SetItem( vars, 1, PyLong_FromLong(      m_ResponderID             ) );
        PyTuple_SetItem( vars, 2, PyUnicode_FromString( m_CoordinatorName.c_str() ) );
        PyTuple_SetItem( vars, 3, PyLong_FromLong(      numVectorsSampled         ) );
        PyTuple_SetItem( vars, 4, py_allele_combos );
        PyTuple_SetItem( vars, 5, py_fractions );

        PyObject* retVal = PyObject_CallObject( pFunc, vars );
        if( retVal == nullptr )
        {
            PyErr_Print();
            ostringstream msg;
            msg << "Failed to call python function '" << PYTHON_FUNC_NAME_RESPOND << "'. ";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        if( !PyList_Check( retVal ) )
        {
            PyErr_Print();
            ostringstream msg;
            msg << "Failed to return values from function '" << PYTHON_FUNC_NAME_RESPOND << "'. ";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        Py_ssize_t list_size = PyList_Size( retVal );
        for( Py_ssize_t i = 0; i < list_size; ++i )
        {
            PyObject* py_str = PyList_GetItem( retVal, i );
            if( !PyUnicode_Check( py_str ) )
            {
                PyErr_Print();
                ostringstream msg;
                msg << "Failed getting data from python function '" << PYTHON_FUNC_NAME_RESPOND << "'. ";
                throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            PyObject* py_str2 = PyUnicode_AsASCIIString( py_str );
            if( !PyBytes_Check( py_str2 ) )
            {
                PyErr_Print();
                ostringstream msg;
                msg << "Failed converting to ASCII from python function '" << PYTHON_FUNC_NAME_RESPOND << "'. ";
                throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
                        
            std::string name = PyBytes_AsString( py_str2 );
            event_names.push_back( name );

            // It looks like I don't need to decrement the reference counter from PyList_GetItem
            Py_DECREF(py_str2);
        }

        Py_DECREF(vars);
        Py_DECREF(retVal);

        for (std::string& r_name : event_names)
        {
            EventTriggerCoordinator trigger(r_name);
            m_pSim->GetCoordinatorEventBroadcaster()->TriggerObservers(m_Parent, trigger);
        }

        if (!m_SurveyCompletedEvent.IsUninitialized())
        {
            m_pSim->GetCoordinatorEventBroadcaster()->TriggerObservers(m_Parent, m_SurveyCompletedEvent);
        }
#endif
        return (event_names.size() > 0);
    }

    // ------------------------------------------------------------------------
    // --- VectorSurveillanceEventCoordinator
    // ------------------------------------------------------------------------

    IMPLEMENT_FACTORY_REGISTERED(VectorSurveillanceEventCoordinator)

    BEGIN_QUERY_INTERFACE_BODY( VectorSurveillanceEventCoordinator )
        HANDLE_INTERFACE( IEventCoordinatorEventContext )
        HANDLE_INTERFACE( ICoordinatorEventObserver )
        HANDLE_INTERFACE( IConfigurable )
        HANDLE_INTERFACE( IEventCoordinator )
        HANDLE_ISUPPORTS_VIA( IEventCoordinator )
    END_QUERY_INTERFACE_BODY( VectorSurveillanceEventCoordinator )

    VectorSurveillanceEventCoordinator::VectorSurveillanceEventCoordinator()
        : JsonConfigurable()
        , m_Parent( nullptr )
        , m_pCounter( new VectorCounter() )
        , m_pResponder( new VectorResponder() )
        , m_CoordinatorName( "VectorSurveillanceEventCoordinator" )
        , m_StopTriggerConditionList()
        , m_StartTriggerConditionList()
        , m_Duration(-1.0f)
        , m_CachedNodes()
        , m_IsExpired( false )
        , m_DurationExpired(false)
        , m_HasBeenDistributed( false )
        , m_IsActive(false)
        , m_IsStarting(false)
        , m_IsStopping(false)
    {
    }

    VectorSurveillanceEventCoordinator::~VectorSurveillanceEventCoordinator()
    {
        delete m_pCounter;
        delete m_pResponder;
    }

    QuickBuilder VectorSurveillanceEventCoordinator::GetSchema()
    {
        return QuickBuilder( jsonSchemaBase );
    }

    bool VectorSurveillanceEventCoordinator::Configure( const Configuration* inputJson )
    {
        initConfigTypeMap( "Coordinator_Name",  &m_CoordinatorName, Coordinator_Name_DESC_TEXT, m_CoordinatorName.c_str() );
        initConfigTypeMap( "Counter",           m_pCounter,         VSEC_Counter_DESC_TEXT );
        initConfigTypeMap( "Responder",         m_pResponder,       VSEC_Responder_DESC_TEXT );

        initConfigTypeMap( "Start_Trigger_Condition_List", &m_StartTriggerConditionList, VSEC_Start_Trigger_Condition_List_DESC_TEXT );
        initConfigTypeMap( "Stop_Trigger_Condition_List",  &m_StopTriggerConditionList,  VSEC_Stop_Trigger_Condition_List_DESC_TEXT );
        initConfigTypeMap( "Duration",                     &m_Duration,                  VSEC_Duration_DESC_TEXT, -1.0f, FLT_MAX, -1.0f );

        bool retValue = JsonConfigurable::Configure(inputJson);
        if (retValue && !JsonConfigurable::_dryrun)
        {
            CheckConfigurationTriggers();
            if( !m_pCounter->HasBeenConfigured() )
            {
                std::ostringstream msg;
                msg << "In Coordinator \'" << m_CoordinatorName << "', the 'Counter' has not been configured\n";
                msg << "Please verify that the 'Counter' dictionary is present.";
                throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__, msg.str().c_str());
            }
        }

        return retValue;
    }

    void VectorSurveillanceEventCoordinator::CheckConfigurationTriggers()
    {
        //check if the same trigger is not defined as start and stop condition
        for (EventTriggerCoordinator& etc : m_StopTriggerConditionList)
        {
            auto found = find(m_StartTriggerConditionList.begin(), m_StartTriggerConditionList.end(), etc);
            if (found != m_StartTriggerConditionList.end())
            {
                std::ostringstream msg;
                msg << "In Coordinator \'" << m_CoordinatorName << "\' stop trigger \'" << etc.ToString() << "\' is already defined in Start_Trigger_Condition_List.";
                throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__, msg.str().c_str());
            }
        }
    }

    void VectorSurveillanceEventCoordinator::SetContextTo( ISimulationEventContext* isec )
    {
        m_Parent = isec;
        m_pResponder->SetContextTo( m_Parent, this );
        Register();
    }

    void VectorSurveillanceEventCoordinator::Update( float dt )
    {
        if( m_Duration != -1.0f )
        {
            if( m_Duration <= 0 )
            {
                m_DurationExpired = true;
            }
            m_Duration -= dt;
        }        
        if( m_IsStarting )
        {
            m_IsStarting = false;
            m_IsActive = true;
            m_pCounter->StartCounting();
        }
        if( m_IsStopping )
        {
            m_IsStopping = false;
            m_IsActive = false;
        }

    }

    void VectorSurveillanceEventCoordinator::Register()
    {
        //Register Start and Stop events
        for( EventTriggerCoordinator& ect : m_StartTriggerConditionList )
        {
            m_Parent->GetCoordinatorEventBroadcaster()->RegisterObserver( this, ect );
            LOG_INFO_F("%s: registered Start_Trigger: %s\n", m_CoordinatorName.c_str(), ect.ToString().c_str());
        }
        for( EventTriggerCoordinator& ect : m_StopTriggerConditionList )
        {
            m_Parent->GetCoordinatorEventBroadcaster()->RegisterObserver(this, ect);
            LOG_INFO_F("%s: registered Stop_Trigger: %s\n", m_CoordinatorName.c_str(), ect.ToString().c_str());
        }
    }

    bool VectorSurveillanceEventCoordinator::notifyOnEvent( IEventCoordinatorEventContext* pEntity,
                                                            const EventTriggerCoordinator& trigger )
    {
        auto it_start = find( m_StartTriggerConditionList.begin(), m_StartTriggerConditionList.end(), trigger );
        if( it_start != m_StartTriggerConditionList.end() )
        {
            LOG_INFO_F("%s: notifyOnEvent received Start: %s\n", m_CoordinatorName.c_str(), trigger.ToString().c_str());
            m_IsStarting = true;
        }
        else
        {
            LOG_INFO_F("%s: notifyOnEvent received Stop: %s\n", m_CoordinatorName.c_str(), trigger.ToString().c_str());
            m_IsStopping = true;
        }
        return true;
    }

    void VectorSurveillanceEventCoordinator::Unregister()
    {
        for( EventTriggerCoordinator& ect : m_StartTriggerConditionList )
        {
            m_Parent->GetCoordinatorEventBroadcaster()->UnregisterObserver( this, ect );
            LOG_INFO_F("%s: Unregistered Start_Trigger: %s\n", m_CoordinatorName.c_str(), ect.ToString().c_str());
        }
        for( EventTriggerCoordinator& ect : m_StopTriggerConditionList )
        {
            m_Parent->GetCoordinatorEventBroadcaster()->UnregisterObserver( this, ect );
            LOG_INFO_F("%s: Unregistered Stop_Trigger: %s\n", m_CoordinatorName.c_str(), ect.ToString().c_str());
        }
    }

    bool VectorSurveillanceEventCoordinator::IsFinished()
    {
        if( m_DurationExpired )
        {
            Unregister();
            m_IsExpired = true;
        }
        return m_DurationExpired;
    }

    void VectorSurveillanceEventCoordinator::ConsiderResponding()
    {
        if( !m_IsActive ) return;

        if( m_pCounter->IsDoneCounting() )
        {
            m_pCounter->CollectStatistics( m_CachedNodes );

            m_pResponder->Respond( m_pCounter->GetNumVectorsSampled(),
                                   m_pCounter->GetNames(),
                                   m_pCounter->GetFractions() );

            m_pCounter->ResetCounting();
        }
    }

    void VectorSurveillanceEventCoordinator::AddNode( const suids::suid& node_suid )
    {
        INodeEventContext* pNEC = m_Parent->GetNodeEventContext( node_suid );
        m_CachedNodes.push_back( pNEC );
    }

    void VectorSurveillanceEventCoordinator::UpdateNodes( float dt )
    {
        if( m_IsActive )
        {
            m_pCounter->Update( dt );
            ConsiderResponding();
        }
    }

    IEventCoordinatorEventContext* VectorSurveillanceEventCoordinator::GetEventContext()
    {
        return this;
    }

    const std::string& VectorSurveillanceEventCoordinator::GetName() const
    {
        return m_CoordinatorName;
    }

    const IdmDateTime& VectorSurveillanceEventCoordinator::GetTime() const
    {
        release_assert( m_Parent );
        return m_Parent->GetSimulationTime();
    }

    IEventCoordinator* VectorSurveillanceEventCoordinator::GetEventCoordinator()
    {
        return this;
    }
}