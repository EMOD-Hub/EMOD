
#include "stdafx.h"
#include "VectorGene.h"
#include "Exceptions.h"
#include "Log.h"
#include "Debug.h"
#include "IdmString.h"

SETUP_LOGGING( "VectorGene" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- VectorAlleleMutation
    // ------------------------------------------------------------------------

    VectorAlleleMutation::VectorAlleleMutation( const std::set<std::string>* pAlleleNameSet )
        : JsonConfigurable()
        , m_pAlleleNameSet( pAlleleNameSet )
        , m_MutateFrom()
        , m_MutateTo()
        , m_AlleleIndexTo(0)
        , m_Frequency(0.0f)
    {
    }

    VectorAlleleMutation::~VectorAlleleMutation()
    {
    }

    bool VectorAlleleMutation::Configure( const Configuration* config )
    {
        jsonConfigurable::ConstrainedString tmp_from;
        tmp_from.constraint_param = m_pAlleleNameSet;
        tmp_from.constraints = "Genes[X].Alleles[X].Name";

        jsonConfigurable::ConstrainedString tmp_to;
        tmp_to.constraint_param = m_pAlleleNameSet;
        tmp_to.constraints = "Genes[X].Alleles[X].Name";

        initConfigTypeMap( "Mutate_From",             &tmp_from,    VAM_Mutate_From_DESC_TEXT,             std::string("") );
        initConfigTypeMap( "Mutate_To",               &tmp_to,      VAM_Mutate_To_DESC_TEXT,               std::string("") );
        initConfigTypeMap( "Probability_Of_Mutation", &m_Frequency, VAM_Probability_Of_Mutation_DESC_TEXT, 0.0f, 1.0f, 0.0f );

        bool ret = JsonConfigurable::Configure( config );
        if( ret && !JsonConfigurable::_dryrun )
        {
            m_MutateFrom = tmp_from;
            m_MutateTo = tmp_to;
            if( m_MutateFrom.empty() )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__,
                                                 "'Mutate_From' of an allele is empty string.\nAllele names must be defined and unique for the entire species." );
            }
            if( m_MutateTo.empty() )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__,
                                                 "'Mutate_To' of an allele is empty string.\nAllele names must be defined and unique for the entire species." );
            }
            if( m_MutateFrom == m_MutateTo )
            {
                std::stringstream ss;
                ss << "'Mutate_From' cannot equal 'Mutate_To'(=" << m_MutateTo << ").\n";
                ss << "Nothing will change for an allele that mutates to itself.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
        return ret;
    }

    const std::string& VectorAlleleMutation::GetAlleleNameFrom() const
    {
        return m_MutateFrom;
    }

    const std::string& VectorAlleleMutation::GetAlleleNameTo() const
    {
        return m_MutateTo;
    }

    void VectorAlleleMutation::SetAlleleIndexTo( uint8_t indexTo )
    {
        m_AlleleIndexTo = indexTo;
    }

    uint8_t VectorAlleleMutation::GetAlleleIndexTo() const
    {
        return m_AlleleIndexTo;
    }

    float VectorAlleleMutation::GetFrequency() const
    {
        return m_Frequency;
    }
    
    // ------------------------------------------------------------------------
    // --- VectorAlleleMutationCollection
    // ------------------------------------------------------------------------

    VectorAlleleMutationCollection::VectorAlleleMutationCollection( VectorAlleleCollection& rPossibleAlleles )
        : JsonConfigurableCollection( "Mutations" )
        , m_rPossibleAlleles( rPossibleAlleles )
    {
    }

    VectorAlleleMutationCollection::~VectorAlleleMutationCollection()
    {

    }

    void VectorAlleleMutationCollection::CheckConfiguration()
    {
        for( int i = 0; i < m_Collection.size(); ++i )
        {
            VectorAlleleMutation* p_mutation = m_Collection[ i ];

            bool found_from = false;
            for( int j = 0; !found_from && (j < m_rPossibleAlleles.Size()); ++j )
            {
                VectorAllele* p_allele_from = m_rPossibleAlleles[ j ];
                if( p_allele_from == nullptr ) continue;

                if( p_allele_from->GetName() == p_mutation->GetAlleleNameFrom() )
                {
                    found_from = true;
                    bool found_to = false;
                    for( int k = 0; !found_to && (k < m_rPossibleAlleles.Size()); ++k )
                    {
                        VectorAllele* p_allele_to = m_rPossibleAlleles[ k ];
                        if( p_allele_to == nullptr ) continue;

                        if( p_allele_to->GetName() == p_mutation->GetAlleleNameTo() )
                        {
                            found_to = true;
                            p_mutation->SetAlleleIndexTo( p_allele_to->GetIndex() );
                            p_allele_from->AddMutation( p_mutation );
                        }
                    }
                    release_assert( found_to );
                }
            }
            release_assert( found_from );
        }
        // all of the mutations have been transfered to the allele
        m_Collection.clear();
    }

    VectorAlleleMutation* VectorAlleleMutationCollection::CreateObject()
    {
        // The order added will give the allele its index into the collection
        return new VectorAlleleMutation( m_rPossibleAlleles.GetAlleleNameSet() );
    }

    // ------------------------------------------------------------------------
    // --- VectorAllele
    // ------------------------------------------------------------------------

    VectorAllele::VectorAllele( uint8_t index )
        : JsonConfigurable()
        , m_Name()
        , m_Index( index )
        , m_Frequency()
        , m_IsMaleAllele( false )
        , m_Mutations()
    {
    }

    VectorAllele::VectorAllele( const std::string& rName, uint8_t index, float frequency, bool isMaleAllele )
        : JsonConfigurable()
        , m_Name( rName )
        , m_Index( index )
        , m_Frequency( frequency )
        , m_IsMaleAllele( isMaleAllele )
        , m_Mutations()
    {
    }

    VectorAllele::~VectorAllele()
    {
    }

    bool VectorAllele::Configure( const Configuration* config )
    {
        initConfigTypeMap( "Name",                     &m_Name,         VA_Name_DESC_TEXT,      std::string("") );
        initConfigTypeMap( "Initial_Allele_Frequency", &m_Frequency,    VA_Frequency_DESC_TEXT, 0.0f, 1.0f, 0.0f );
        initConfigTypeMap( "Is_Y_Chromosome",          &m_IsMaleAllele, VA_Is_Y_Chromosome_DESC_TEXT, false );

        bool ret = JsonConfigurable::Configure( config );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( m_Name.empty() )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__,
                                                 "'Name' of an allele is empty string.\nAllele names must be defined and unique for the entire species." );
            }
        }
        return ret;
    }

    const std::string& VectorAllele::GetName() const
    {
        return m_Name;
    }

    void VectorAllele::SetIndex( uint8_t index )
    {
        m_Index = index;
    }
    uint8_t VectorAllele::GetIndex() const
    {
        return m_Index;
    }

    float VectorAllele::GetFrequency() const
    {
        return m_Frequency;
    }

    void VectorAllele::AddMutation( VectorAlleleMutation* pMutation )
    {
        m_Mutations.push_back( pMutation );
    }

    const std::vector<VectorAlleleMutation*>& VectorAllele::GetMutations() const
    {
        return m_Mutations;
    }

    bool VectorAllele::IsFemale() const
    {
        return !m_IsMaleAllele;
    }

    bool VectorAllele::IsMale() const
    {
        return m_IsMaleAllele;
    }

    // ------------------------------------------------------------------------
    // --- VectorAlleleCollection
    // ------------------------------------------------------------------------

    VectorAlleleCollection::VectorAlleleCollection()
        : JsonConfigurableCollection( "Alleles" )
        , m_AlleleNameSet()
    {
    }

    VectorAlleleCollection::~VectorAlleleCollection()
    {

    }

    const std::set<std::string>* VectorAlleleCollection::GetAlleleNameSet() const
    {
        return &m_AlleleNameSet;
    }

    bool compareAllele( const VectorAllele* pLeft, const VectorAllele* pRight )
    {
        return (pLeft->GetName() < pRight->GetName());
    }

    void VectorAlleleCollection::CheckConfiguration()
    {
        if( m_Collection.size()  == 0 )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "The parameter 'Genes[x].Alleles' cannot have zero entries." );
        }
        else if( m_Collection.size() > VectorGamete::MAX_ALLELES )
        {
            std::stringstream ss;
            ss << "The gene/locus has too many alleles defined (" << m_Collection.size() << ").\n";
            ss << "There is a maximum of " << uint32_t(VectorGamete::MAX_ALLELES) << " allele per gene.\n";
            ss << "The alleles defined are:\n";
            for( auto p_allele : m_Collection )
            {
                ss << p_allele->GetName() << "\n";
            }
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        float total_freq = 0.0;
        for( int i = 0; i < m_Collection.size(); ++i )
        {
            m_AlleleNameSet.insert( m_Collection[ i ]->GetName() );

            total_freq += m_Collection[ i ]->GetFrequency();

            for( int j = i + 1; j < m_Collection.size(); ++j )
            {
                if( m_Collection[ i ]->GetName() == m_Collection[ j ]->GetName() )
                {
                    std::stringstream ss;
                    ss << "Duplicate allele name - '" << m_Collection[ i ]->GetName() << "'\n";
                    ss << "Allele names must be unique for the entire species.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
            }
        }

        if( fabs( total_freq - 1.0 ) > FLT_EPSILON )
        {
            std::stringstream ss;
            ss << "Genes[x].Alleles frequencies do not sum to one.  The frequencies are:\n";
            for( auto p_allele : m_Collection )
            {
                ss << "'" << p_allele->GetName() << "' = " << p_allele->GetFrequency() << "\n";
            }
            ss << "Total Frequency = " << total_freq << "\n";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        // -------------------------------------------------------------------
        // --- Sort the allele so that they are in alphabetical order by name.
        // --- This ensures that the results are the same regardless of the
        // --- order of the allele in the input.
        // -------------------------------------------------------------------
        std::sort( m_Collection.begin(), m_Collection.end(), compareAllele );
        for( int index = 0; index < m_Collection.size(); ++index )
        {
            m_Collection[ index ]->SetIndex( uint8_t(index) );
        }
    }

    VectorAllele* VectorAlleleCollection::CreateObject()
    {
        // The order added will give the allele its index into the collection
        return new VectorAllele( m_Collection.size() );
    }

    // ------------------------------------------------------------------------
    // --- VectorGene
    // ------------------------------------------------------------------------

    VectorGene::VectorGene( bool isGenderGene )
        : JsonConfigurable()
        , m_IsGenderGene( isGenderGene )
        , m_LocusIndex( 0 )
        , m_PossibleAllele()
    {
        if( m_IsGenderGene )
        {
            SetLocusIndex( VectorGenome::GENDER_LOCUS_INDEX );
            AddAllele( "X", 0.75, false );
            AddAllele( "Y", 0.25, true  );
            ConfigureGenderAlleles();
        }
    }

    VectorGene::~VectorGene()
    {
    }

    bool VectorGene::Configure( const Configuration* config )
    {
        VectorAlleleMutationCollection tmp_mutations( m_PossibleAllele );

        initConfigTypeMap( "Is_Gender_Gene", &m_IsGenderGene, VG_Is_Gender_Gene_DESC_TEXT, false );

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! NOTE:  The order that these are read in is important.  Alleles must be read in first so
        // !!! that Mutations can verify the that its alleles are valid.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        initConfigComplexCollectionType( "Alleles", &m_PossibleAllele, VG_Allele_DESC_TEXT );
        bool ret = JsonConfigurable::Configure( config );
        if( ret && !JsonConfigurable::_dryrun )
        {
            // See Exist( "Mutations" ) below
            if( !config->Exist( "Alleles" ) )
            {
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "Missing parameter 'Alleles' in parameter 'Genes'" );
            }
            m_PossibleAllele.CheckConfiguration();

            if( m_IsGenderGene )
            {
                ConfigureGenderAlleles();
            }
        }

        initConfigComplexCollectionType( "Mutations", &tmp_mutations, VG_Mutations_DESC_TEXT );

        if( ret )
        {
            ret = JsonConfigurable::Configure( config );
            if( ret && !JsonConfigurable::_dryrun )
            {
                // ---------------------------------------------------------------------------------
                // --- I'm putting these checks here because the handling of missing parameters
                // --- doesn't occur until all of the parameters have been read.  However,
                // --- VectorTraitModifiers depends on these parameters being read correctly.
                // --- If these parameters are misssing, we can get a message from VectorTraitModifiers
                // --- saying it can't find an allele when this was the problem.
                // ---------------------------------------------------------------------------------
                if( !config->Exist( "Mutations" ) )
                {
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "Missing parameter 'Mutations' in parameter 'Genes'" );
                }
                tmp_mutations.CheckConfiguration();
            }
        }
        return ret;
    }

    void VectorGene::ConfigureGenderAlleles()
    {
        if( m_PossibleAllele.Size() <= 1 )
        {
            std::stringstream ss;
            ss << "'Is_Gender_Gene' is set to true but both 'X' and 'Y' alleles are not defined.\n";
            ss << "If you are including the gender 'gene', then it must be the first in the list and both 'X' and 'Y' must be defined.";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        // ----------------------------------------------------------------------------
        // --- Separate out the female and male alleles so that we can put them back
        // --- such that the location in the array matches the desired index value.
        // --- We want female alleles to have index values from 0-3 and males from 4-7.
        // --- This makes the 3rd bit 0 for females and 1 for males.
        // ----------------------------------------------------------------------------
        std::vector<VectorAllele*> female_alleles;
        std::vector<VectorAllele*> male_alleles;
        for( int i = 0; i < m_PossibleAllele.Size(); ++i )
        {
            VectorAllele* p_allele = m_PossibleAllele[ i ];
            if( p_allele->IsFemale() )
            {
                female_alleles.push_back( p_allele );
            }
            else
            {
                male_alleles.push_back( p_allele );
            }
        }

        // ---------------------------------------------------------------
        // --- Verify we have at least one X and one Y and not too many of one gender
        // ---------------------------------------------------------------
        if( female_alleles.size() == 0 )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__,
                                             "'Is_Gender_Gene' is set to true but zero X-chromosomes have been defined." );
        }
        if( male_alleles.size() == 0 )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__,
                                             "'Is_Gender_Gene' is set to true but zero Y-chromosomes have been defined." );
        }
        if( (female_alleles.size() > VectorGamete::GENDER_MAX_ALLELES) ||
            (male_alleles.size()   > VectorGamete::GENDER_MAX_ALLELES) )
        {
            std::string gender = "female";
            int num = female_alleles.size();
            if( male_alleles.size() > VectorGamete::GENDER_MAX_ALLELES )
            {
                gender = "male";
                num = male_alleles.size();
            }
            std::stringstream ss;
            ss << "Invalid number of alleles for one gender.\n";
            ss << num << " " << gender << " alleles were defined.  There can only be at most 4 of each gender.";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        // --------------------------------------------------------------
        // --- Sort the allele so that input order doesn't impact results
        // --------------------------------------------------------------
        std::sort( female_alleles.begin(), female_alleles.end(), compareAllele );
        std::sort( male_alleles.begin(),   male_alleles.end(),   compareAllele );

        // --------------------------------------------------------
        // --- Add the females such that X is in the zero index and
        // --- the others are in indexes 1-3.
        // --------------------------------------------------------
        m_PossibleAllele.Clear();
        for( auto p_allele : female_alleles )
        {
            m_PossibleAllele.Add( p_allele );
        }
        for( int i = female_alleles.size(); i < VectorGamete::GENDER_MAX_ALLELES; ++i )
        {
            m_PossibleAllele.Add( nullptr );
        }

        // --------------------------------------------------------
        // --- Add the males such that Y is in the 4th index and
        // --- the others are in indexes 5-7.
        // --------------------------------------------------------
        for( auto p_allele : male_alleles )
        {
            m_PossibleAllele.Add( p_allele );
        }
        for( int index = 0; index < m_PossibleAllele.Size(); ++index )
        {
            if( m_PossibleAllele[ index ] != nullptr )
            {
                m_PossibleAllele[ index ]->SetIndex( uint8_t(index) );
            }
        }
    }

    bool VectorGene::IsGenderGene() const
    {
        return m_IsGenderGene;
    }

    void VectorGene::SetLocusIndex( uint8_t locusIndex )
    {
        m_LocusIndex = locusIndex;
    }

    uint8_t VectorGene::GetLocusIndex() const
    {
        return m_LocusIndex;
    }

    uint8_t VectorGene::GetNumAllele() const
    {
        return uint8_t(m_PossibleAllele.Size());
    }

    const VectorAllele* VectorGene::GetAllele( uint8_t index ) const
    {
        return m_PossibleAllele[ index ];
    }

    VectorAllele& VectorGene::GetAllele( const std::string& rName )
    {
        VectorAllele* p_allele = GetAllelePointer( rName );
        if( p_allele != nullptr )
        {
            return *p_allele;
        }
        else
        {
            std::stringstream ss;
            ss << "Invalid Allele name for this Gene = '" << rName << "'";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    void VectorGene::AddAllele( const std::string& rName, float frequency, bool isMaleAllele )
    {
        VectorAllele* p_allele = new VectorAllele( rName, m_PossibleAllele.Size(), frequency, isMaleAllele );
        m_PossibleAllele.Add( p_allele );
    }

    bool VectorGene::IsValidAlleleName( const std::string& rName )
    {
        return (GetAllelePointer( rName ) != nullptr);
    }

    std::string VectorGene::GetPossibleAlleleNames() const
    {
        std::stringstream ss;
        ss << "[";
        for( int i = 0; i < m_PossibleAllele.Size(); ++i )
        {
            if( m_PossibleAllele[ i ] != nullptr )
            {
                ss << "'" << m_PossibleAllele[ i ]->GetName() << "'";
                if( (i + 1) < m_PossibleAllele.Size() )
                {
                    ss << ",";
                }
            }
        }
        ss << "]";
        return ss.str();
    }

    VectorAllele* VectorGene::GetAllelePointer( const std::string& rName )
    {
        for( int i = 0; i < m_PossibleAllele.Size(); ++i )
        {
            if( (m_PossibleAllele[ i ] != nullptr) && (m_PossibleAllele[ i ]->GetName() == rName) )
            {
                return m_PossibleAllele[ i ];
            }
        }
        return nullptr;
    }

    // ------------------------------------------------------------------------
    // --- VectorGeneCollection
    // ------------------------------------------------------------------------

    VectorGeneCollection::VectorGeneCollection()
        : JsonConfigurableCollection( "Genes" )
        , m_AlleleNamesDefined()
        , m_GenderGeneAlleleNames()
        , m_AlleleNameToGeneMap()
    {
    }

    VectorGeneCollection::~VectorGeneCollection()
    {
    }

    void VectorGeneCollection::CheckConfiguration()
    {
        if( this->m_Collection.size() > VectorGamete::MAX_LOCI )
        {
            std::stringstream ss;
            ss << m_Collection.size() << " vector genes have been defined and the maximum is " << uint32_t(VectorGamete::MAX_LOCI);
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        // ---------------------------------------------------------------------
        // --- if the first gene is not the gender gene, shift all of the genes
        // --- to the back of the array and add gender gene to the front
        // ---------------------------------------------------------------------
        release_assert( VectorGenome::GENDER_LOCUS_INDEX == 0 );
        if( (m_Collection.size() == 0) || !m_Collection[ 0 ]->IsGenderGene() )
        {
            m_Collection.push_back( nullptr );
            for( int index = m_Collection.size() - 1; index >= 1; --index )
            {
                m_Collection[ index ] = m_Collection[ index - 1 ];
            }
            m_Collection[ 0 ] = new VectorGene( true );
            release_assert( m_Collection[0]->GetLocusIndex() == VectorGenome::GENDER_LOCUS_INDEX );
        }

        for( uint8_t locus_index = 0 ; locus_index < m_Collection.size() ; ++locus_index )
        {
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! Notice that the locus index and the index in the collection of Genes are the same.
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            VectorGene* p_gene = m_Collection[ locus_index ];
            p_gene->SetLocusIndex( locus_index );

            for( uint8_t allele_index = 0; allele_index < p_gene->GetNumAllele(); ++allele_index )
            {
                const VectorAllele* p_allele = p_gene->GetAllele( allele_index );
                if( p_allele == nullptr ) continue;

                if( m_AlleleNameToGeneMap.count( p_allele->GetName() ) > 0 )
                {
                    std::stringstream ss;
                    if( (p_allele->GetName() == "X") || (p_allele->GetName() == "Y") )
                    {
                        ss << "Allele names 'X' and 'Y' are only allowed for the Gender 'gene'.\n";
                        ss << "If you are including the gender 'gene', then it must be the first in the list and both alleles defined.";
                    }
                    else
                    {
                        ss << "There is more than one allele with the name '" << p_allele->GetName() << "'.\n";
                        ss << "Allele names must be unique.";
                    }
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                m_AlleleNameToGeneMap.insert( std::make_pair( p_allele->GetName(), p_gene ) );
                m_AlleleNamesDefined.insert( p_allele->GetName() );
                if( locus_index == VectorGenome::GENDER_LOCUS_INDEX )
                {
                    m_GenderGeneAlleleNames.insert( p_allele->GetName() );
                }
            }
        }
        release_assert( m_AlleleNamesDefined.size() == m_AlleleNameToGeneMap.size() );
    }

    bool VectorGeneCollection::IsValidAlleleName( const std::string& rAlleleName ) const
    {
        return ( m_AlleleNameToGeneMap.count( rAlleleName ) > 0 );
    }

    uint8_t VectorGeneCollection::GetLocusIndex( const std::string& rAlleleName ) const
    {
        if( m_AlleleNameToGeneMap.count( rAlleleName ) == 0 )
        {
            std::stringstream ss;
            ss << "The allele name ='" << rAlleleName << "' was not found in the map.";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        VectorGene* p_gene = m_AlleleNameToGeneMap.at( rAlleleName );
        return p_gene->GetLocusIndex();
    }

    uint8_t VectorGeneCollection::GetAlleleIndex( const std::string& rAlleleName ) const
    {
        if( m_AlleleNameToGeneMap.count( rAlleleName ) == 0 )
        {
            std::stringstream ss;
            ss << "The allele name ='" << rAlleleName << "' was not found in the map.";
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        VectorGene* p_gene = m_AlleleNameToGeneMap.at( rAlleleName );
        return p_gene->GetAllele( rAlleleName ).GetIndex();
    }

    const std::set<std::string>& VectorGeneCollection::GetDefinedAlleleNames() const
    {
        return m_AlleleNamesDefined;
    }

    const std::set< std::string >& VectorGeneCollection::GetGenderGeneAlleleNames() const
    {
        return m_GenderGeneAlleleNames;
    }

    void VectorGeneCollection::ConvertAlleleCombinationsStrings( const std::string& rParameterName,
                                                                 const std::vector<std::vector<std::string>>& rComboStrings,
                                                                 VectorGameteBitPair_t* pBitMask,
                                                                 std::vector<VectorGameteBitPair_t>* pPossibleGenomes ) const
    {
        release_assert( pBitMask != nullptr );
        release_assert( pPossibleGenomes != nullptr );

        if( rComboStrings.size() == 0 )
        {
            std::stringstream ss;
            ss << "The parameter '" << rParameterName << "' has no combinations.\n";
            ss << "You must define at least one combination.";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        if( rComboStrings.size() > m_Collection.size() )
        {
            std::stringstream ss;
            ss << "The parameter '" << rParameterName << "' has too many combinations.\n";
            ss << "'" << rParameterName << "' = [";
            for( int i = 0; i < rComboStrings.size(); ++i )
            {
                ss << "[";
                for( int j = 0; j < rComboStrings[ i ].size(); ++j )
                {
                    ss << "'" << rComboStrings[ i ][ j ] << "'";
                    if( (j + 1) < rComboStrings[ i ].size() )
                    {
                        ss << ",";
                    }
                }
                ss << "]";
                if( (i + 1) < rComboStrings.size() )
                {
                    ss << ",";
                }
            }
            ss << "]\n";
            ss << "The following genes/loci and their alleles are defined:\n";
            for( auto p_gene : m_Collection )
            {
                ss << p_gene->GetPossibleAlleleNames() << "\n";
            }
            ss << "'" << rParameterName << "' can define at most one allele pair per locus.";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        VectorGamete gamete_bit_mask;
        std::set<uint8_t> locus_indexes_used;
        std::vector<VectorGenome> possible_genomes;
        possible_genomes.push_back( VectorGenome() );

        for( int i = 0 ; i < rComboStrings.size() ; ++i )
        {
            std::vector<AlleleIndexes> indexes_list = ConvertStringsToIndexes( rParameterName,
                                                                               i,
                                                                               locus_indexes_used,
                                                                               rComboStrings[ i ] );
            release_assert( indexes_list.size() > 0 );

            // Set all of the bits at this locus in the mask.
            gamete_bit_mask.SetLocus( indexes_list[0].locus_index, VectorGamete::MAX_ALLELES - 1 );
            locus_indexes_used.insert( indexes_list[ 0 ].locus_index );

            std::vector<VectorGenome> tmp_possible_genomes = possible_genomes;
            possible_genomes.clear();

            for( auto indexes : indexes_list )
            {
                for( VectorGenome genome : tmp_possible_genomes )
                {
                    genome.SetLocus( indexes.locus_index,
                                     indexes.allele_index_1,
                                     indexes.allele_index_2 );
                    possible_genomes.push_back( genome );
                }

                // ---------------------------------------------------------------------------------
                // --- Get the opposite order of the alleles
                // --- If gene is the gender gender gene, we want to get opposites of X chromosomes,
                // --- but not Y chromosomes since the mom gamete cannot have Y chromosomes.
                // ---------------------------------------------------------------------------------
                if( (indexes.allele_index_1 != indexes.allele_index_2) &&
                    ( ((indexes.locus_index == 0) && (indexes.allele_index_2 < VectorGamete::GENDER_BIT_MASK)) || (indexes.locus_index != 0) ) )
                {
                    for( VectorGenome genome : tmp_possible_genomes )
                    {
                        genome.SetLocus( indexes.locus_index,
                                         indexes.allele_index_2,
                                         indexes.allele_index_1 );
                        possible_genomes.push_back( genome );
                    }
                }
            }
        }

        (*pBitMask) = VectorGamete::Convert( gamete_bit_mask, gamete_bit_mask );

        pPossibleGenomes->clear();
        for( auto genome : possible_genomes )
        {
            pPossibleGenomes->push_back( genome.GetBits() );
        }
    }

    VectorGenome VectorGeneCollection::CreateGenome( const std::string& rParameterName,
                                                     const std::vector<std::vector<std::string>>& rComboStrings ) const
    {
        if( rComboStrings.size() != m_Collection.size() )
        {
            std::stringstream ss;
            ss << "The parameter '" << rParameterName <<"' does not have an allele pair for each gene/loci.\n";
            ss << "The parameter has " << rComboStrings.size() << " pairs and there are " << (m_Collection.size()-1) << " defined plus the gender gene.";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        std::set<uint8_t> locus_indexes_used;
        VectorGenome genome;
        for( int i = 0; i < rComboStrings.size(); ++i )
        {
            std::vector<AlleleIndexes> indexes_list = ConvertStringsToIndexes( rParameterName,
                                                                               i,
                                                                               locus_indexes_used,
                                                                               rComboStrings[i] );
            if( indexes_list.size() != 1 )
            {
                std::stringstream ss;
                ss << "The parameter '" << rParameterName << "' has the allele pair #" << (i + 1) << " with\n";
                ss << "'*' in one of the pair.  You cannot use the '*' symbol in this parameter.\n";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            genome.SetLocus( indexes_list[0].locus_index,
                             indexes_list[0].allele_index_1,
                             indexes_list[0].allele_index_2 );

            locus_indexes_used.insert( indexes_list[0].locus_index );
        }

        return genome;
    }

    std::vector<VectorGeneCollection::AlleleIndexes>
    VectorGeneCollection::ConvertStringsToIndexes( const std::string& rParameterName,
                                                   int allelePairIndex,
                                                   const std::set<uint8_t>& rLocusIndexesUsed,
                                                   const std::vector<std::string>& rAllelePairStrings ) const
    {
        // --------------------------------------------------------
        // --- Our genome has two gametes/chromosomes so we expect
        // --- the alleles to come in paris per gene.
        // --------------------------------------------------------
        if( rAllelePairStrings.size() != 2 )
        {
            std::stringstream ss;
            ss << "The parameter '" << rParameterName << "' has the allele pair #" << (allelePairIndex + 1) << " with\n";
            ss << rAllelePairStrings.size() << " elements instead of two.";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        const std::string& r_allele_name_1 = rAllelePairStrings[ 0 ];
        const std::string& r_allele_name_2 = rAllelePairStrings[ 1 ];

        if( (r_allele_name_1 != "*") && !IsValidAlleleName( r_allele_name_1 ) )
        {
            std::stringstream ss;
            ss << "The parameter '" << rParameterName << "' has the allele pair #" << (allelePairIndex + 1) << " with\n";
            ss << "'" << r_allele_name_1 << "' that is not an allele in one of the 'Genes'.";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        
        if( (r_allele_name_2 != "*") && !IsValidAlleleName( r_allele_name_2 ) )
        {
            std::stringstream ss;
            ss << "The parameter '" << rParameterName << "' has the allele pair #" << (allelePairIndex + 1) << " with\n";
            ss << "'" << r_allele_name_2 << "' that is not an allele in one of the 'Genes'.";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        
        std::vector<AlleleIndexes> indexes_list;
        AlleleIndexes indexes;
        if( (r_allele_name_1 == "*") && (r_allele_name_2 == "*") )
        {
            std::stringstream ss;
            ss << "The parameter '" << rParameterName << "' has the allele pair #" << (allelePairIndex + 1) << " with\n";
            ss << "'*' and '*'.  You can only use the '*' for one allele of the pair.\n";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        else if( (r_allele_name_1 != "*") && (r_allele_name_2 != "*") )
        {
            indexes.locus_index    = GetLocusIndex(  r_allele_name_1 );
            indexes.allele_index_1 = GetAlleleIndex( r_allele_name_1 );
            indexes.allele_index_2 = GetAlleleIndex( r_allele_name_2 );
            uint8_t locus_index_2  = GetLocusIndex(  r_allele_name_2 );
            if( indexes.locus_index != locus_index_2 )
            {
                std::stringstream ss;
                ss << "The parameter '" << rParameterName << "' has the allele pair #" << (allelePairIndex + 1) << " with\n";
                ss << "allele '" << r_allele_name_1 << "' and '" << r_allele_name_2 << "' and they are not from the same Gene.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            if( indexes.locus_index == 0 )
            {
                const VectorAllele* p_allele_1 = m_Collection[ indexes.locus_index ]->GetAllele( indexes.allele_index_1 );
                const VectorAllele* p_allele_2 = m_Collection[ indexes.locus_index ]->GetAllele( indexes.allele_index_2 );

                // special handling for the gender gene
                if( p_allele_1->IsMale() && p_allele_2->IsMale() )
                {
                    std::stringstream ss;
                    ss << "The parameter '" << rParameterName << "' has the allele pair #" << (allelePairIndex + 1) << " with\n";
                    ss << "allele 'Y' and 'Y'.  The gender locus must have one 'X'.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                else if( p_allele_1->IsMale() && p_allele_2->IsFemale() )
                {
                    uint8_t tmp = indexes.allele_index_1;
                    indexes.allele_index_1 = indexes.allele_index_2;
                    indexes.allele_index_2 = tmp;
                }
            }
            indexes_list.push_back( indexes );
        }
        else if( r_allele_name_1 == "*" )
        {
            indexes.locus_index    = GetLocusIndex(  r_allele_name_2 );
            indexes.allele_index_2 = GetAlleleIndex( r_allele_name_2 );

            // special handling for the gender gene
            if( indexes.locus_index == 0 )
            {
                const VectorAllele* p_allele_2 = m_Collection[ indexes.locus_index ]->GetAllele( indexes.allele_index_2 );

                for( uint8_t index = 0; index < m_Collection[ indexes.locus_index ]->GetNumAllele(); ++index )
                {
                    const VectorAllele* p_allele_i = m_Collection[ indexes.locus_index ]->GetAllele( index );
                    if( p_allele_i == nullptr ) continue;

                    if( p_allele_2->IsFemale() || (p_allele_2->IsMale() && p_allele_i->IsFemale()) )
                    {
                        indexes.allele_index_1 = index;
                        if( p_allele_i->IsMale() )
                        {
                            uint8_t tmp = indexes.allele_index_1;
                            indexes.allele_index_1 = indexes.allele_index_2;
                            indexes.allele_index_2 = tmp;
                        }
                        indexes_list.push_back( indexes );
                    }
                }
            }
            else
            {
                for( uint8_t index = 0; index < m_Collection[ indexes.locus_index ]->GetNumAllele(); ++index )
                {
                    indexes.allele_index_1 = index;
                    indexes_list.push_back( indexes );
                }
            }
        }
        else //if( r_allele_name_2 == "*" )
        {
            indexes.locus_index = GetLocusIndex( r_allele_name_1 );
            indexes.allele_index_1 = GetAlleleIndex( r_allele_name_1 );

            // special handling for the gender gene
            if( indexes.locus_index == 0 )
            {
                const VectorAllele* p_allele_1 = m_Collection[ indexes.locus_index ]->GetAllele( indexes.allele_index_1 );

                for( uint8_t index = 0; index < m_Collection[ indexes.locus_index ]->GetNumAllele(); ++index )
                {
                    const VectorAllele* p_allele_i = m_Collection[ indexes.locus_index ]->GetAllele( index );
                    if( p_allele_i == nullptr ) continue;

                    if( p_allele_1->IsFemale() || (p_allele_1->IsMale() && p_allele_i->IsFemale()) )
                    {
                        indexes.allele_index_2 = index;
                        if( p_allele_1->IsMale() )
                        {
                            uint8_t tmp = indexes.allele_index_1;
                            indexes.allele_index_1 = indexes.allele_index_2;
                            indexes.allele_index_2 = tmp;
                        }
                        indexes_list.push_back( indexes );
                    }
                }
            }
            else
            {
                for( uint8_t index = 0; index < m_Collection[ indexes.locus_index ]->GetNumAllele(); ++index )
                {
                    indexes.allele_index_2 = index;
                    indexes_list.push_back( indexes );
                }
            }
        }

        if( rLocusIndexesUsed.find( indexes.locus_index ) != rLocusIndexesUsed.end() )
        {
            std::stringstream ss;
            ss << "The parameter '" << rParameterName << "' has the allele pair #" << (allelePairIndex + 1) << " with\n";
            ss << "alleles [\'" << r_allele_name_1 << "','" << r_allele_name_2 << "'] from a Gene already defined.\n";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        return indexes_list;
    }

    std::string VectorGeneCollection::GetGenomeName( const VectorGenome& rGenome ) const
    {
        std::string mom_gamete_name;
        std::string dad_gamete_name;
        for( auto p_gene : m_Collection )
        {
            uint8_t locus_index = p_gene->GetLocusIndex();
            
            std::pair<uint8_t,uint8_t> allele_indexes = rGenome.GetLocus( locus_index );

            const VectorAllele* p_mom_allele = p_gene->GetAllele( allele_indexes.first );
            const VectorAllele* p_dad_allele = p_gene->GetAllele( allele_indexes.second );
            release_assert( p_mom_allele != nullptr );
            release_assert( p_dad_allele != nullptr );

            if( !mom_gamete_name.empty() )
            {
                mom_gamete_name += "-";
                dad_gamete_name += "-";
            }
            mom_gamete_name += p_mom_allele->GetName();
            dad_gamete_name += p_dad_allele->GetName();
        }
        VectorGenome tmp = rGenome;
        LOG_VALID_F("Vector gametes %s=%d %s=%d\n",
            mom_gamete_name.c_str(), tmp.GetGamete(VectorGenomeGameteIndex::GAMETE_INDEX_MOM).GetBits(),
            dad_gamete_name.c_str(), tmp.GetGamete(VectorGenomeGameteIndex::GAMETE_INDEX_DAD).GetBits());
        return mom_gamete_name + ":" + dad_gamete_name;
    }

    const std::string& VectorGeneCollection::GetAlleleName( uint8_t locusIndex, uint8_t alleleIndex ) const
    {
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Assuming locus index is the same as in the collection
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        const VectorAllele* p_allele = m_Collection[ locusIndex ]->GetAllele( alleleIndex );
        release_assert( p_allele != nullptr );
        return p_allele->GetName();
    }

    VectorGene* VectorGeneCollection::CreateObject()
    {
        return new VectorGene();
    }
}
