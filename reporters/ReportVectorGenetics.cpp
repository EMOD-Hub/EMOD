
#include "stdafx.h"

#include "ReportVectorGenetics.h"

#include "report_params.rc"
#include "INodeContext.h"
#include "NodeEventContext.h"
#include "Individual.h"
#include "VectorContexts.h"
#include "ReportUtilities.h"
#include "IdmDateTime.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "ReportVectorGenetics" ) // <<< Name of this file

namespace Kernel
{
    // ----------------------------------------
    // --- AlleleComboCount Methods
    // ----------------------------------------
    AlleleComboCount::AlleleComboCount()
        : name()
        , allele_name_list()
        , vector_pop(0)
        , state_counts()
        , vector_pop_death(0)
        , vector_pop_death_sum_age(0.0f)
        , death_counts()
        , sum_age()
    {
    }

    AlleleComboCount::AlleleComboCount( const std::string& rAlleleName )
        : AlleleComboCount( std::vector<std::string>{rAlleleName} )
    {
    }

    AlleleComboCount::AlleleComboCount( const std::vector<std::string>& rAlleleNameList )
        : name()
        , allele_name_list( rAlleleNameList )
        , vector_pop( 0 )
        , state_counts( VectorStateEnum::pairs::count() , 0 )
        , vector_pop_death( 0 )
        , vector_pop_death_sum_age( 0.0f )
        , death_counts( VectorStateEnum::pairs::count(), 0 )
        , sum_age( VectorStateEnum::pairs::count(), 0.0f )
    {
        release_assert( allele_name_list.size() > 0 );
        name = allele_name_list[0];
        for( int i = 1; i < allele_name_list.size(); ++i )
        {
            name += "." + allele_name_list[i];
        }
    }

    AlleleComboCount::AlleleComboCount( const AlleleComboCount& rThat )
        : name( rThat.name )
        , allele_name_list( rThat.allele_name_list )
        , vector_pop( rThat.vector_pop )
        , state_counts( rThat.state_counts )
        , vector_pop_death( rThat.vector_pop_death )
        , vector_pop_death_sum_age( rThat.vector_pop_death_sum_age )
        , death_counts( rThat.death_counts )
        , sum_age( rThat.sum_age )
    {
    }

    AlleleComboCount::~AlleleComboCount()
    {
    }

    bool AlleleComboCount::IsInGenome( const std::string& rGenomeName ) const
    {
        // ----------------------------------------------------------
        // --- the genome must contain all of the allele in the list,
        // --- but they can be in any order.
        // ----------------------------------------------------------
        for( auto allele_name : allele_name_list )
        {
            if( rGenomeName.find( allele_name ) == std::string::npos )
            {
                return false;
            }
        }
        return true;
    }

    bool AlleleComboCount::IsHomozygous( const std::string& rGenomeName ) const
    {
        if( allele_name_list.size() != 1 )
        {
            return false;
        }
        std::string allele_name = allele_name_list[ 0 ];
        size_t index_found = rGenomeName.find( allele_name );
        if( index_found == std::string::npos )
        {
            return false;
        }
        // +1 so it doesn't find the first occurrence
        index_found = rGenomeName.find( allele_name, index_found+1 );
        if( index_found == std::string::npos )
        {
            return false;
        }
        return true;
    }

    // ----------------------------------------
    // --- RVG_AlleleCombo Methods
    // ----------------------------------------

    RVG_AlleleCombo::RVG_AlleleCombo()
        : JsonConfigurable()
        , m_ComboStrings()
    {
    }

    RVG_AlleleCombo::~RVG_AlleleCombo()
    {
    }

    bool RVG_AlleleCombo::Configure( const Configuration* inputJson )
    {
        // We can't validate the values at this point because we don't have access
        // to the gene object yet.
        initConfigTypeMap( "Allele_Combination", &m_ComboStrings, RVG_Allele_Combination_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( inputJson );

        return ret;
    }

    const std::vector<std::vector<std::string>>& RVG_AlleleCombo::GetComboStrings() const
    {
        return m_ComboStrings;
    }

    // ----------------------------------------
    // --- AlleleComboCollection Methods
    // ----------------------------------------

    AlleleComboCollection::AlleleComboCollection()
        : JsonConfigurableCollection( "vector AlleleCombo" )
    {
    }

    AlleleComboCollection::~AlleleComboCollection()
    {
    }

    void AlleleComboCollection::CheckConfiguration()
    {
    }

    RVG_AlleleCombo* AlleleComboCollection::CreateObject()
    {
        return new RVG_AlleleCombo();
    }

    // ----------------------------------------
    // --- ReportVectorGenetics Methods
    // ----------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED( ReportVectorGenetics, BaseTextReport )
        HANDLE_INTERFACE( IReport )
    END_QUERY_INTERFACE_DERIVED( ReportVectorGenetics, BaseTextReport )

#ifndef _REPORT_DLL
    IMPLEMENT_FACTORY_REGISTERED( ReportVectorGenetics )
#endif

    ReportVectorGenetics::ReportVectorGenetics()
        : ReportVectorGenetics( "ReportVectorGenetics.csv" )
    {
    }

    ReportVectorGenetics::ReportVectorGenetics( const std::string& rReportName )
        : BaseTextReport( rReportName )
        , m_IncludeVectorStateColumns( true )
        , m_IncludeDeathByStateColumns( false )
        , m_CombineSimilarGenomes(false)
        , m_HasStratificationData(false)
        , m_StratifyBy( StratifyBy::GENOME )
        , m_SpecificGenomeCombination()
        , m_GenomeHasAlleleCombination()
        , m_Species()
        , m_Gender( VectorGender::VECTOR_FEMALE )
        , m_PossibleGenomes()
        , m_AlleleComboCounts()
        , m_GenomeToAlleleComboCountVector()
        , m_AllelesForStratification()
        , m_ReportFilter( nullptr, "", false, false, false )
    {
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();

        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
    }

    ReportVectorGenetics::~ReportVectorGenetics()
    {
    }

    bool ReportVectorGenetics::Configure( const Configuration * inputJson )
    {
        m_ReportFilter.ConfigureParameters( *this, inputJson );

        initConfigTypeMap( "Species", &m_Species, RVG_Species_DESC_TEXT );
        initConfig( "Gender",
                    m_Gender,
                    inputJson,
                    MetadataDescriptor::Enum( "Gender", RVG_Gender_DESC_TEXT, MDD_ENUM_ARGS( VectorGender ) ) );

        initConfigTypeMap( "Include_Vector_State_Columns", 
                           &m_IncludeVectorStateColumns,
                           RVG_Include_Vector_State_Columns_DESC_TEXT,
                           true );

        initConfigTypeMap( "Include_Death_By_State_Columns",
                           &m_IncludeDeathByStateColumns,
                           RVG_Include_Death_By_State_Columns_DESC_TEXT,
                           false );

        initConfig( "Stratify_By",
                    m_StratifyBy,
                    inputJson,
                    MetadataDescriptor::Enum( "Stratify_By", RVG_Stratify_By_DESC_TEXT, MDD_ENUM_ARGS( StratifyBy ) ) );

        initConfigTypeMap( "Combine_Similar_Genomes",
                           &m_CombineSimilarGenomes,
                           RVG_Combine_Similar_Genomes_DESC_TEXT,
                           false,
                           "Stratify_By", "GENOME,SPECIFIC_GENOME" );

        initConfigComplexCollectionType( "Specific_Genome_Combinations_For_Stratification",
                               &m_SpecificGenomeCombination,
                               RVG_Specific_Genome_Combinations_For_Stratification_DESC_TEXT,
                               "Stratify_By", "SPECIFIC_GENOME" );

        initConfigTypeMap( "Allele_Combinations_For_Stratification",
                           &m_GenomeHasAlleleCombination,
                           RVG_Allele_Combinations_For_Stratification_DESC_TEXT,
                           (const char *)nullptr,
                           JsonConfigurable::empty_set,
                           "Stratify_By", "ALLELE" );

        initConfigTypeMap( "Alleles_For_Stratification",
                           &m_AllelesForStratification,
                           RVG_Alleles_For_Stratification_DESC_TEXT,
                           (const char *)nullptr,
                           JsonConfigurable::empty_set,
                           "Stratify_By", "ALLELE_FREQ" );

        bool ret = JsonConfigurable::Configure( inputJson );

        if( ret && !JsonConfigurable::_dryrun )
        {
            CheckSpecificGenomeCombination();
            m_ReportFilter.CheckParameters( inputJson );
            UpdateReportName();
        }
        return ret;
    }

    void ReportVectorGenetics::UpdateReportName()
    {
        std::string gender = "";
        if( m_Gender == VectorGender::VECTOR_FEMALE )
        {
            gender = "Female";
        }
        else if( m_Gender == VectorGender::VECTOR_MALE )
        {
            gender = "Male";
        }

        std::string name = GetReportName();
        name = name.substr( 0, name.size() - 4 );//-4 is number of characters in ".csv"

        std::string species_str = "";
        if( m_Species.empty() )
        {
            LOG_WARN( "'Species' is undefined.  Will just do the first species.\n" );
        }
        else
        {
            species_str = "_" + m_Species;
        }

        std::string new_name = name + species_str;
        if( gender.length() > 0 )
        {
            new_name += "_" + gender;
        }
        new_name += std::string( "_" ) + std::string( StratifyBy::pairs::lookup_key( m_StratifyBy ) );
        new_name = m_ReportFilter.GetNewReportName( new_name );
        new_name += ".csv";

        SetReportName( new_name );
    }

    void ReportVectorGenetics::CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& nodeIds_demographics )
    {
        m_ReportFilter.CheckForValidNodeIDs( GetReportName(), nodeIds_demographics );
    }

    void ReportVectorGenetics::CheckSpecificGenomeCombination()
    {
        if( m_StratifyBy == StratifyBy::SPECIFIC_GENOME )
        {
            if( m_SpecificGenomeCombination.Size() == 0 )
            {
                std::stringstream ss;
                ss << "'Stratify_By' = 'SPECIFIC_GENOME' and 'Specific_Genome_Combinations_For_Stratification' has zero elements.\n";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            else if( m_Gender != VectorGender::VECTOR_BOTH_GENDERS )
            {
                for( int i = 0; i < m_SpecificGenomeCombination.Size(); ++i )
                {
                    const std::vector<std::vector<std::string>>& r_combo = m_SpecificGenomeCombination[ i ]->GetComboStrings();
                    for( int j = 0; j < r_combo.size(); ++j )
                    {
                        if( (m_Gender == VectorGender::VECTOR_FEMALE) &&
                            ((r_combo[ j ][ 0 ] == "Y") || (r_combo[ j ][ 1 ] == "Y")) )
                        {
                            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__,
                                                             "'Gender'='VECTOR_FEMALE' and one of the combinations in 'Specific_Genome_Combinations_For_Stratification' is for a male (i.e. contains 'Y')." );
                        }
                        else if( (m_Gender == VectorGender::VECTOR_MALE) &&
                            ((r_combo[ j ][ 0 ] == "X") && (r_combo[ j ][ 1 ] == "X")) )
                        {
                            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__,
                                                             "'Gender'='VECTOR_MALE' and one of the combinations in 'Specific_Genome_Combinations_For_Stratification' is for a female (i.e. contains a pair of 'X' allele)." );
                        }
                    }
                }
            }
        }
    }

    bool ReportVectorGenetics::IncludeVectorStateColumn( VectorStateEnum::Enum state ) const
    {
        bool include = false;
        switch( state )
        {
            case VectorStateEnum::STATE_EGG:
            case VectorStateEnum::STATE_LARVA:
            case VectorStateEnum::STATE_IMMATURE:
                include = m_IncludeVectorStateColumns;
                break;
            case VectorStateEnum::STATE_MALE:
                include = ((m_Gender == VectorGender::VECTOR_MALE) ||
                    (m_Gender == VectorGender::VECTOR_BOTH_GENDERS));
                break;
            case VectorStateEnum::STATE_ADULT:
            case VectorStateEnum::STATE_INFECTIOUS:
            case VectorStateEnum::STATE_INFECTED:
                include = ((m_Gender == VectorGender::VECTOR_FEMALE) ||
                    (m_Gender == VectorGender::VECTOR_BOTH_GENDERS))
                    && m_IncludeVectorStateColumns;
                break;
        }
        return include;
    }

    std::string ReportVectorGenetics::GetHeader() const
    {
        std::stringstream header ;

        header      << "Time"
             << "," << "NodeID";

        if( (m_StratifyBy == StratifyBy::ALLELE) || (m_StratifyBy == StratifyBy::ALLELE_FREQ) )
        {
            header << "," << "Alleles";
        }
        else
        {
            header << "," << "Genome";
        }

        if( (m_Gender == VectorGender::VECTOR_FEMALE      ) ||
            (m_Gender == VectorGender::VECTOR_BOTH_GENDERS) )
        {
            header << "," << "VectorPopulation";
        }
        for( int i = 0; i < VectorStateEnum::pairs::count(); ++i )
        {
            bool include = IncludeVectorStateColumn( VectorStateEnum::Enum(VectorStateEnum::pairs::get_values()[ i ]) );
            if( include )
            {
                header << "," << VectorStateEnum::pairs::get_keys()[ i ];
            }
        }
        if( m_IncludeDeathByStateColumns )
        {
            if( (m_Gender == VectorGender::VECTOR_FEMALE) ||
                (m_Gender == VectorGender::VECTOR_BOTH_GENDERS) )
            {
                header << "," << "VectorPopulationNumDied";
            }

            if( IncludeVectorStateColumn( VectorStateEnum::STATE_INFECTIOUS ) )
            {
                header << "," << "InfectiousNumDied";
            }
            if( IncludeVectorStateColumn( VectorStateEnum::STATE_INFECTED ) )
            {
                header << "," << "InfectedNumDied";
            }
            if( IncludeVectorStateColumn( VectorStateEnum::STATE_ADULT ) )
            {
                header << "," << "AdultNumDied";
            }
            if( IncludeVectorStateColumn( VectorStateEnum::STATE_MALE ) )
            {
                header << "," << "MaleNumDied";
            }

            if( (m_Gender == VectorGender::VECTOR_FEMALE) ||
                (m_Gender == VectorGender::VECTOR_BOTH_GENDERS) )
            {
                header << "," << "VectorPopulationAvgAgeAtDeath";
            }

            if( IncludeVectorStateColumn( VectorStateEnum::STATE_INFECTIOUS ) )
            {
                header << "," << "InfectiousAvgAgeAtDeath";
            }
            if( IncludeVectorStateColumn( VectorStateEnum::STATE_INFECTED ) )
            {
                header << "," << "InfectedAvgAgeAtDeath";
            }
            if( IncludeVectorStateColumn( VectorStateEnum::STATE_ADULT ) )
            {
                header << "," << "AdultAvgAgeAtDeath";
            }
            if( IncludeVectorStateColumn( VectorStateEnum::STATE_MALE ) )
            {
                header << "," << "MaleAvgAgeAtDeath";
            }
        }
        return header.str();
    }

    void ReportVectorGenetics::LogNodeData( Kernel::INodeContext* pNC )
    {
        auto time      = pNC->GetTime().time ;
        auto nodeId    = pNC->GetExternalID();
        auto node_suid = pNC->GetSuid();
        
        INodeVector * pNodeVector = NULL;
        if (s_OK != pNC->QueryInterface(GET_IID(INodeVector), (void**)&pNodeVector) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pNC", "INodeVector", "INodeContext");
        }

        const VectorPopulationReportingList_t& vector_pop_list = pNodeVector->GetVectorPopulationReporting();
        if( vector_pop_list.size() == 0 )
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                 "Invalid attempt to use 'ReportVectorGenetics'.\nThere are no vector species(i.e. 'Vector_Species_Params') defined so the report will do nothing." );
        }

        // ---------------------------------------------------------------------
        // --- if the list is empty, then we want to include all of the species
        // ---------------------------------------------------------------------
        if( m_Species.empty() )
        {
            auto it = vector_pop_list.begin();
            m_Species = (*it)->get_SpeciesID();
        }

        if (!m_HasStratificationData)
        {
            for (auto vp : vector_pop_list)
            {
                if (m_Species != vp->get_SpeciesID())
                {
                    continue;
                }
                InitializeStratificationData(vp);
            }
        }

        if (!m_ReportFilter.IsValidTime(pNC->GetTime())) return;
        if (!m_ReportFilter.IsValidNode(pNC->GetEventContext())) return;

        for( auto vp : vector_pop_list )
        {
            const std::string& species = vp->get_SpeciesID();

            if( m_Species != species )
            {
                continue;
            }

            std::vector<NameToColumnData> row_data;
            if( (m_StratifyBy == StratifyBy::ALLELE) || (m_StratifyBy == StratifyBy::ALLELE_FREQ) )
            {
                row_data = CollectDataByAllele( vp );
            }
            else
            {
                row_data = CollectDataByGenome( vp );
            }

            for( auto& column_data : row_data )
            {
                GetOutputStream()
                           << time
                    << "," << nodeId
                    << "," << column_data.name;
                for( auto count : column_data.counts )
                {
                    GetOutputStream() << "," << count;
                }
                for( auto age : column_data.avg_age )
                {
                    GetOutputStream() << "," << age;
                }
                for( auto other : column_data.other )
                {
                    GetOutputStream() << "," << other;
                }
                GetOutputStream() << endl;
            }
        }

        ResetOtherCounters();
    }

    std::vector<PossibleGenome> ReportVectorGenetics::FindSpecificGenomes( IVectorPopulationReporting* pIVPR )
    {
        int num_genes = pIVPR->GetNumGenes();

        // ---------------------------------------------------------
        // --- Convert the user input into a set of possible genomes
        // ---------------------------------------------------------
        std::map<VectorGenome, VectorGenome> used_map;
        std::vector<PossibleGenome> user_possible_genomes;
        for( int i = 0; i < m_SpecificGenomeCombination.Size(); ++i )
        {
            std::stringstream param_name_ss;
            param_name_ss << "Specific_Genome_Combinations_For_Stratification[ " << i << " ]";

            if( m_SpecificGenomeCombination[ i ]->GetComboStrings().size() != num_genes )
            {
                std::stringstream ss;
                ss << "Invalid Genome Combination Defined in " << GetReportName() << " Configuration\n";
                ss << param_name_ss.str() << " defines " << m_SpecificGenomeCombination[ i ]->GetComboStrings().size() << " genes it must define all " << num_genes;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            GenomeNamePairVector_t genomes_for_combo;
            pIVPR->ConvertAlleleCombinationsStrings( param_name_ss.str().c_str(),
                                                     m_SpecificGenomeCombination[ i ]->GetComboStrings(),
                                                     &genomes_for_combo );

            for( auto gnp : genomes_for_combo )
            {
                if( used_map.count( gnp.genome ) == 0 )
                {
                    user_possible_genomes.push_back( PossibleGenome( gnp ) );
                    used_map[ gnp.genome ] = gnp.genome;
                }
            }
        }

        // ----------------------------------------------------------
        // --- Of the genomes found based on the user specifications,
        // --- combine any of these that are similar
        // ----------------------------------------------------------
        std::vector<PossibleGenome> net_possible_genomes = pIVPR->CombinePossibleGenomes( m_CombineSimilarGenomes,
                                                                                          user_possible_genomes );

        // ----------------------------------------------------------------
        // --- Create the "Other" column by finding the genomes not created
        // --- from the user specifications and calling them all "similar"
        // ----------------------------------------------------------------
        std::vector<PossibleGenome> all_possible_genomes = pIVPR->GetAllPossibleGenomes( m_Gender );
        PossibleGenome pg_other;
        for( auto pg : all_possible_genomes )
        {
            if( used_map.count( pg.genome ) == 0 )
            {
                if( pg_other.name.empty() )
                {
                    pg_other.name = "Other";
                    pg_other.genome = pg.genome;
                }
                else
                {
                    pg_other.similar_genomes.push_back( pg.genome );
                }
            }
        }
        if( !pg_other.name.empty() )
        {
            net_possible_genomes.push_back( pg_other );
        }

        return net_possible_genomes;
    }

    void CheckAlleleName( const std::set<std::string>& rPossibleAlleleNames, const std::string rAlleleName, const char* pParamName )
    {
        if( rPossibleAlleleNames.find( rAlleleName ) == rPossibleAlleleNames.end() )
        {
            std::stringstream ss;
            ss << "The allele = '" << rAlleleName << "' specified in '"<< pParamName << "' is unknown.\n";
            ss << "Please specify only alleles defined in 'Genes'";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    std::vector<AlleleComboCount> ReportVectorGenetics::CreateAlleleComboCounts( IVectorPopulationReporting* pIVPR )
    {
        std::vector<AlleleComboCount> combo_counts;

        // ------------------------------------------------------------------------------------
        // --- If we are collecting data per allele, then we first create the AlleleComboCounts
        // --- since these are the things that are collecting the data and will be the things
        // --- written out.
        // ------------------------------------------------------------------------------------
        std::set<std::string> possible_allele_names = pIVPR->GetPossibleAlleleNames();

        if( (m_StratifyBy == StratifyBy::ALLELE_FREQ) && (m_AllelesForStratification.size() > 0) )
        {
            // ------------------------------------------------------------------------------
            // --- If collecting allele frequencies and the user provided a list of alleles,
            // --- replace the possible list with those from the user.
            // ------------------------------------------------------------------------------
            std::set<std::string> tmp_names = possible_allele_names;
            possible_allele_names.clear();
            for( auto allele_name : m_AllelesForStratification )
            {
                CheckAlleleName( tmp_names, allele_name, "Alleles_For_Stratification" );
                possible_allele_names.insert( allele_name );
            }
        }

        for( auto allele_name : possible_allele_names )
        {
            combo_counts.push_back( AlleleComboCount( allele_name ) );
        }

        // ----------------------------------------------------------------------
        // --- If the user provided allele combinations, add these combinations
        // --- to the list to be counting.
        // ----------------------------------------------------------------------
        for( auto allele_name_list : m_GenomeHasAlleleCombination )
        {
            std::set<uint8_t> used_loci;
            for( auto allele_name : allele_name_list )
            {
                CheckAlleleName( possible_allele_names, allele_name, "Allele_Combinations_For_Stratification" );
                uint8_t locus = pIVPR->GetLocusIndex( allele_name );
                if( used_loci.find( locus ) != used_loci.end() )
                {
                    std::stringstream ss;
                    ss << "The allele = '" << allele_name << "' specified in 'Allele_Combinations_For_Stratification' is the second allele at locus " << uint32_t(locus) << ".\n";
                    ss << "You can only specify one allele per locus.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                used_loci.insert( locus );
            }
            combo_counts.push_back( AlleleComboCount( allele_name_list ) );
        }

        return combo_counts;
    }

    void ReportVectorGenetics::LinkGenomesToAlleleComboCounts( 
        const std::vector<PossibleGenome>& rPossibleGenomes,
        const std::vector<AlleleComboCount>& rAlleleComboCounts,
        std::vector<GenomeToAlleleComboCount>& rGenomeToAlleleComboCountVector )
    {
        // -------------------------------------------------------------------------
        // --- Since the data is actually stored per genome, we create a structure
        // --- that links the genomes to the allele combos of interest.
        // -------------------------------------------------------------------------
        for( auto& r_entry : rPossibleGenomes )
        {
            GenomeToAlleleComboCount gtac;
            gtac.genome_name = r_entry.name;
            gtac.genome = r_entry.genome;
            for( int i = 0; i < rAlleleComboCounts.size(); ++i )
            {
                if( m_AlleleComboCounts[ i ].IsInGenome( gtac.genome_name ) )
                {
                    gtac.allele_counts.push_back( &(m_AlleleComboCounts[ i ]) );

                    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                    // !!! ALLELE_FREQ puts the AlleleComboCount in the list twice when the genome
                    // !!! is homozygous in the allele.  This should cause the allele to get counted
                    // !!! twice when it is homozygous.
                    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                    if( (m_StratifyBy == StratifyBy::ALLELE_FREQ) &&
                        m_AlleleComboCounts[ i ].IsHomozygous( gtac.genome_name ) )
                    {
                        gtac.allele_counts.push_back( &(m_AlleleComboCounts[ i ]) );
                    }
                }
            }
            rGenomeToAlleleComboCountVector.push_back( gtac );
        }
    }

    void ReportVectorGenetics::InitializeStratificationData( IVectorPopulationReporting* pIVPR )
    {
        if( m_StratifyBy == StratifyBy::SPECIFIC_GENOME )
        {
            m_PossibleGenomes = FindSpecificGenomes( pIVPR );
        }
        else
        {
            m_PossibleGenomes = pIVPR->FindPossibleGenomes( m_Gender, m_CombineSimilarGenomes );
            if( (m_StratifyBy == StratifyBy::ALLELE) || (m_StratifyBy == StratifyBy::ALLELE_FREQ) )
            {
                m_AlleleComboCounts = CreateAlleleComboCounts( pIVPR );
                LinkGenomesToAlleleComboCounts( m_PossibleGenomes,
                                                m_AlleleComboCounts,
                                                m_GenomeToAlleleComboCountVector );
            }
        }

        m_HasStratificationData = true;
    }

    NameToColumnData ReportVectorGenetics::CreateColumnData( const std::string& rName,
                                                             uint32_t vectorPop,
                                                             const std::vector<uint32_t>& rStateCounts,
                                                             uint32_t vectorPopDeath,
                                                             float vectorPopSumAge,
                                                             const std::vector<uint32_t>& rDeathCount,
                                                             const std::vector<float>& rSumAge )
    {
        NameToColumnData column_data;
        column_data.name = rName;
        if( (m_Gender == VectorGender::VECTOR_FEMALE      ) ||
            (m_Gender == VectorGender::VECTOR_BOTH_GENDERS) )
        {
            column_data.counts.push_back( vectorPop );
        }
        for( int i = 0; i < rStateCounts.size(); ++i )
        {
            if( IncludeVectorStateColumn( VectorStateEnum::Enum( VectorStateEnum::pairs::get_values()[ i ] ) ) )
            {
                column_data.counts.push_back( rStateCounts[ i ] );
            }
        }
        if( m_IncludeDeathByStateColumns )
        {
            if( (m_Gender == VectorGender::VECTOR_FEMALE) ||
                (m_Gender == VectorGender::VECTOR_BOTH_GENDERS) )
            {
                column_data.counts.push_back( vectorPopDeath );
                if( vectorPopDeath > 0 )
                {
                    column_data.avg_age.push_back( vectorPopSumAge / float( vectorPopDeath ) );
                }
                else
                {
                    column_data.avg_age.push_back( 0.0f );
                }
            }

            for( int i = 0; i < rDeathCount.size(); ++i )
            {
                if( (i <= VectorStateEnum::STATE_MALE) && 
                    IncludeVectorStateColumn( VectorStateEnum::Enum( VectorStateEnum::pairs::get_values()[ i ] ) ) )
                {
                    column_data.counts.push_back( rDeathCount[ i ] );
                    if( rDeathCount[ i ] > 0 )
                    {
                        column_data.avg_age.push_back( rSumAge[ i ] / float( rDeathCount[ i ] ) );
                    }
                    else
                    {
                        column_data.avg_age.push_back( 0.0f );
                    }
                }
            }
        }
        return column_data;
    }

    std::vector<NameToColumnData> ReportVectorGenetics::CollectDataByAllele( IVectorPopulationReporting* pIVPR )
    {
        // ----------------------------------------------------------------
        // --- Clear counters so we can just add the data from each genome
        // ----------------------------------------------------------------
        for( auto& r_acc : m_AlleleComboCounts )
        {
            r_acc.vector_pop = 0;
            for( auto& r_sc : r_acc.state_counts )
            {
                r_sc = 0;
            }
            r_acc.vector_pop_death = 0;
            r_acc.vector_pop_death_sum_age = 0.0f;
            for( auto& r_dc : r_acc.death_counts )
            {
                r_dc = 0;
            }
            for( auto& r_sum : r_acc.sum_age )
            {
                r_sum = 0.0f;
            }
        }

        // ------------------------------------------------------------------------
        // --- For each Genome, get data out of VectorPopulation and add that data
        // --- to each allele combo that is in this genome.
        // ------------------------------------------------------------------------
        for( auto& r_gtac : m_GenomeToAlleleComboCountVector )
        {
            std::vector<uint32_t> state_counts;
            std::vector<uint32_t> death_counts;
            std::vector<float> sum_age;
            for( int i = 0; i < VectorStateEnum::pairs::count(); ++i )
            {
                state_counts.push_back( pIVPR->getGenomeCount( VectorStateEnum::Enum( i ), r_gtac.genome ) );
                if( m_IncludeDeathByStateColumns && (i <= int(VectorStateEnum::STATE_MALE)) )
                {
                    death_counts.push_back( pIVPR->getDeathCount(    VectorStateEnum::Enum( i ), r_gtac.genome ) );
                    sum_age.push_back(      pIVPR->getSumAgeAtDeath( VectorStateEnum::Enum( i ), r_gtac.genome ) );
                }
            }

            uint32_t vector_pop = state_counts[ VectorStateEnum::STATE_INFECTIOUS ]
                                + state_counts[ VectorStateEnum::STATE_INFECTED ]
                                + state_counts[ VectorStateEnum::STATE_ADULT ];

            uint32_t vector_pop_death = 0;
            float vector_pop_death_sum_age = 0.0;
            if( m_IncludeDeathByStateColumns )
            {
                vector_pop_death = death_counts[ VectorStateEnum::STATE_INFECTIOUS ]
                                 + death_counts[ VectorStateEnum::STATE_INFECTED ]
                                 + death_counts[ VectorStateEnum::STATE_ADULT ];

                vector_pop_death_sum_age = sum_age[ VectorStateEnum::STATE_INFECTIOUS ]
                                         + sum_age[ VectorStateEnum::STATE_INFECTED ]
                                         + sum_age[ VectorStateEnum::STATE_ADULT ];
            }

            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! When using ALLELE_FREQ, the AlleleComboCount is in the list twice
            // !!! so that it receives counts for both occurrences
            // !!! When using ALLELE, we are just counting the vectors that have the
            // !!! allele.  ALLELE_FREQ is counting the occurrences of the allele.
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // update each AlleleCombo that is in this genome
            for( auto p_acc : r_gtac.allele_counts )
            {
                p_acc->vector_pop += vector_pop;
                p_acc->vector_pop_death += vector_pop_death;
                p_acc->vector_pop_death_sum_age += vector_pop_death_sum_age;
                for( int i = 0; i < state_counts.size(); ++i )
                {
                    p_acc->state_counts[ i ] += state_counts[ i ];
                    if( m_IncludeDeathByStateColumns && (i <= int( VectorStateEnum::STATE_MALE )) )
                    {
                        p_acc->death_counts[ i ] += death_counts[ i ];
                        p_acc->sum_age[ i ] += sum_age[ i ];
                    }
                }
            }
        }

        // ------------------------------------------------------------
        // --- Convert the allele combo counts into the output format
        // ------------------------------------------------------------
        std::vector<NameToColumnData> row_data;
        for( auto& r_acc : m_AlleleComboCounts )
        {
            NameToColumnData column_data = CreateColumnData( r_acc.name,
                                                             r_acc.vector_pop,
                                                             r_acc.state_counts,
                                                             r_acc.vector_pop_death,
                                                             r_acc.vector_pop_death_sum_age,
                                                             r_acc.death_counts,
                                                             r_acc.sum_age );
            row_data.push_back( column_data );
        }
        return row_data;
    }

    std::vector<NameToColumnData> ReportVectorGenetics::CollectDataByGenome( IVectorPopulationReporting* pIVPR )
    {
        std::vector<NameToColumnData> row_data;

        for( auto& r_entry : m_PossibleGenomes )
        {
            std::vector<uint32_t> state_counts;
            std::vector<uint32_t> death_counts;
            std::vector<float> sum_age;
            for( int i = 0; i < VectorStateEnum::pairs::count(); ++i )
            {
                state_counts.push_back( pIVPR->getGenomeCount( VectorStateEnum::Enum( i ), r_entry.genome ) );
                if( m_IncludeDeathByStateColumns && (i <= int( VectorStateEnum::STATE_MALE )) )
                {
                    death_counts.push_back( pIVPR->getDeathCount(    VectorStateEnum::Enum( i ), r_entry.genome ) );
                    sum_age.push_back(      pIVPR->getSumAgeAtDeath( VectorStateEnum::Enum( i ), r_entry.genome ) );
                }
            }
            for( auto& genome : r_entry.similar_genomes )
            {
                for( int i = 0; i < VectorStateEnum::pairs::count(); ++i )
                {
                    state_counts[i] += pIVPR->getGenomeCount( VectorStateEnum::Enum( i ), genome );
                    if( m_IncludeDeathByStateColumns && (i <= int( VectorStateEnum::STATE_MALE )) )
                    {
                        death_counts[i] += pIVPR->getDeathCount(    VectorStateEnum::Enum( i ), genome );
                        sum_age[i]      += pIVPR->getSumAgeAtDeath( VectorStateEnum::Enum( i ), genome );
                    }
                }
            }

            uint32_t vector_pop = state_counts[ VectorStateEnum::STATE_INFECTIOUS ]
                                + state_counts[ VectorStateEnum::STATE_INFECTED ]
                                + state_counts[ VectorStateEnum::STATE_ADULT ];

            uint32_t vector_pop_death = 0;
            float vector_pop_death_sum_age = 0.0;
            if( m_IncludeDeathByStateColumns )
            {
                vector_pop_death = death_counts[ VectorStateEnum::STATE_INFECTIOUS ]
                                 + death_counts[ VectorStateEnum::STATE_INFECTED ]
                                 + death_counts[ VectorStateEnum::STATE_ADULT ];

                vector_pop_death_sum_age = sum_age[ VectorStateEnum::STATE_INFECTIOUS ]
                                         + sum_age[ VectorStateEnum::STATE_INFECTED ]
                                         + sum_age[ VectorStateEnum::STATE_ADULT ];
            }

            NameToColumnData column_data = CreateColumnData( r_entry.name,
                                                             vector_pop,
                                                             state_counts,
                                                             vector_pop_death,
                                                             vector_pop_death_sum_age,
                                                             death_counts,
                                                             sum_age );
            CollectOtherDataByGenome( column_data );

            row_data.push_back( column_data );
        }
        return row_data;
    }
}
