
#include "stdafx.h"
#include "VectorGeneDriver.h"
#include "VectorGene.h"
#include "VectorTraitModifiers.h"
#include "VectorGenome.h"
#include "Exceptions.h"
#include "Log.h"
#include "Debug.h"
#include "IdmString.h"

SETUP_LOGGING( "VectorGeneDriver" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- CopyToAlleleLikelihood
    // ------------------------------------------------------------------------
    
    // public constructor implementations
    CopyToAlleleLikelihood::CopyToAlleleLikelihood( const VectorGeneCollection* pGenes )
        : CopyToAlleleLikelihood( pGenes,
                                  "Copy_To_Allele",
                                  CTAL_Copy_To_Allele_DESC_TEXT,
                                  CTAL_Likelihood_DESC_TEXT )
    {
    }

    CopyToAlleleLikelihood::CopyToAlleleLikelihood( const VectorGeneCollection* pGenes,
                                                    const std::string& rAlleleName,
                                                    uint8_t alleleIndex,
                                                    float likelihood )
        : CopyToAlleleLikelihood( pGenes,
                                  rAlleleName,
                                  alleleIndex,
                                  likelihood,
                                  "Copy_To_Allele",
                                  CTAL_Copy_To_Allele_DESC_TEXT,
                                  CTAL_Likelihood_DESC_TEXT )
    {
    }

    //protected constructor implementations
    CopyToAlleleLikelihood::CopyToAlleleLikelihood( const VectorGeneCollection* pGenes,
                                                    const char* alleleConfigName,
                                                    const char* alleleDescText,
                                                    const char* likelihoodDescText )
        : JsonConfigurable()
        , m_pGenes( pGenes )
        , m_CopyToAlleleName()
        , m_CopyToAlleleIndex( 0 )
        , m_Prob( 0.0f )
        , m_AlleleConfigName( alleleConfigName )
        , m_AlleleDescText( alleleDescText )
        , m_LikelihoodDescText( likelihoodDescText )
    {
    }

    CopyToAlleleLikelihood::CopyToAlleleLikelihood( const VectorGeneCollection* pGenes,
                                                    const std::string& rAlleleName,
                                                    uint8_t alleleIndex,
                                                    float likelihood,
                                                    const char* alleleConfigName,
                                                    const char* alleleDescText,
                                                    const char* likelihoodDescText )
        : JsonConfigurable()
        , m_pGenes( pGenes )
        , m_CopyToAlleleName( rAlleleName )
        , m_CopyToAlleleIndex( alleleIndex )
        , m_Prob( likelihood )
        , m_AlleleConfigName( alleleConfigName )
        , m_AlleleDescText( alleleDescText )
        , m_LikelihoodDescText( likelihoodDescText )
    {
    }

    CopyToAlleleLikelihood::~CopyToAlleleLikelihood()
    {
    }

    bool CopyToAlleleLikelihood::Configure( const Configuration* config )
    {
        const std::set<std::string>& allowed_allele_names = m_pGenes->GetDefinedAlleleNames();

        jsonConfigurable::ConstrainedString name;
        name.constraint_param = &allowed_allele_names;
        name.constraints = m_pGenes->GENE_CONSTRAINTS;
        initConfigTypeMap( m_AlleleConfigName, &name, m_AlleleDescText );

        initConfigTypeMap( "Likelihood", &m_Prob, m_LikelihoodDescText, 0.0f, 1.0f, 0.0f);

        bool is_configured = JsonConfigurable::Configure( config );
        if( is_configured && !JsonConfigurable::_dryrun )
        {
            m_CopyToAlleleName = name;
            m_CopyToAlleleIndex = m_pGenes->GetAlleleIndex( m_CopyToAlleleName );
        }

        return is_configured;
    }

    const std::string& CopyToAlleleLikelihood::GetCopyToAlleleName() const
    {
        return m_CopyToAlleleName;
    }

    uint8_t CopyToAlleleLikelihood::GetCopyToAlleleIndex() const
    {
        return m_CopyToAlleleIndex;
    }

    float CopyToAlleleLikelihood::GetLikelihood() const
    {
        return m_Prob;
    }

    // ------------------------------------------------------------------------
    // --- CopyToAlleleLikelihoodnCollection
    // ------------------------------------------------------------------------


    CopyToAlleleLikelihoodCollection::CopyToAlleleLikelihoodCollection( const VectorGeneCollection* pGenes )
        :CopyToAlleleLikelihoodCollection( pGenes,
                                           "Copy_To_Likelihood" )
    {
    }

    CopyToAlleleLikelihoodCollection::CopyToAlleleLikelihoodCollection( const VectorGeneCollection* pGenes,
                                                                        std::string idmTypeName )
        :  JsonConfigurableCollection<CopyToAlleleLikelihood>(idmTypeName)
        , m_pGenes( pGenes )
    {
    }

    CopyToAlleleLikelihoodCollection::~CopyToAlleleLikelihoodCollection()
    {
    }

    bool compareLikelihoods( const CopyToAlleleLikelihood* pLeft, const CopyToAlleleLikelihood* pRight )
    {
        return (pLeft->GetCopyToAlleleName() < pRight->GetCopyToAlleleName());
    }

    void CopyToAlleleLikelihoodCollection::CheckConfiguration()
    {

        for( int i = 0; i < m_Collection.size(); ++i )
        {
            for( int j = (i+1); j < m_Collection.size(); ++j )
            {
                if( m_Collection[ i ]->GetCopyToAlleleName() == m_Collection[ j ]->GetCopyToAlleleName() )
                {
                    std::stringstream ss;
                    ss << "Duplicate allele name - '" << m_Collection[ i ]->GetCopyToAlleleName() << "' in '" << m_IdmTypeName <<"'\n";
                    ss << "Each allele can be defined only once.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
            }
        }

        // ------------------------------------------------------------------------
        // --- We sort the alleles so that input order does not change the results
        // --- by changing the order of processing.
        // ------------------------------------------------------------------------
        std::sort( m_Collection.begin(), m_Collection.end(), compareLikelihoods );
    }

    CopyToAlleleLikelihood* CopyToAlleleLikelihoodCollection::CreateObject()
    {
        return new CopyToAlleleLikelihood( m_pGenes );
    }

    // ------------------------------------------------------------------------
    // --- AlleleDriven
    // ------------------------------------------------------------------------

    AlleleDriven::AlleleDriven( const VectorGeneCollection* pGenes )
        : JsonConfigurable()
        , m_pGenes( pGenes )
        , m_LocusIndex(0)
        , m_AlleleIndexToCopy(0)
        , m_AlleleIndexToReplace(0)
        , m_ProbabilityOfFailure(0.0)
        , m_CopyToAlleleLikelihoods( pGenes )
    {
    }

    AlleleDriven::AlleleDriven( const VectorGeneCollection* pGenes,
                                uint8_t locusIndex,
                                uint8_t alleleIndexToCopy,
                                uint8_t alleleIndexToReplace )
        : JsonConfigurable()
        , m_pGenes( pGenes )
        , m_LocusIndex( locusIndex )
        , m_AlleleIndexToCopy( alleleIndexToCopy )
        , m_AlleleIndexToReplace( alleleIndexToReplace )
        , m_ProbabilityOfFailure( 0.0 )
        , m_CopyToAlleleLikelihoods( pGenes )
    {
    }

    AlleleDriven::AlleleDriven( const AlleleDriven& rMaster )
        : JsonConfigurable( rMaster )
        , m_pGenes( rMaster.m_pGenes )
        , m_LocusIndex( rMaster.m_LocusIndex )
        , m_AlleleIndexToCopy( rMaster.m_AlleleIndexToCopy )
        , m_AlleleIndexToReplace( rMaster.m_AlleleIndexToReplace )
        , m_ProbabilityOfFailure( rMaster.m_ProbabilityOfFailure )
        , m_CopyToAlleleLikelihoods( rMaster.m_pGenes )
    {
        for( int i = 0; i < rMaster.m_CopyToAlleleLikelihoods.Size(); ++i )
        {
            const CopyToAlleleLikelihood* p_ctal_orig = rMaster.m_CopyToAlleleLikelihoods[ i ];
            CopyToAlleleLikelihood* p_ctal_copy = new CopyToAlleleLikelihood( *p_ctal_orig );
            this->m_CopyToAlleleLikelihoods.Add( p_ctal_copy );
        }
    }

    AlleleDriven::~AlleleDriven()
    {
    }

    bool AlleleDriven::Configure( const Configuration* config )
    {
        const std::set<std::string>& allowed_allele_names = m_pGenes->GetDefinedAlleleNames();

        jsonConfigurable::ConstrainedString to_copy_name;
        to_copy_name.constraint_param = &allowed_allele_names;
        to_copy_name.constraints = m_pGenes->GENE_CONSTRAINTS;
        initConfigTypeMap( "Allele_To_Copy", &to_copy_name, AlleleDriven_Allele_To_Copy_DESC_TEXT );

        jsonConfigurable::ConstrainedString to_replace_name;
        to_replace_name.constraint_param = &allowed_allele_names;
        to_replace_name.constraints = m_pGenes->GENE_CONSTRAINTS;
        initConfigTypeMap( "Allele_To_Replace", &to_replace_name, AlleleDriven_Allele_To_Replace_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( config );
        if( ret && !JsonConfigurable::_dryrun )
        {
            m_LocusIndex = m_pGenes->GetLocusIndex( to_copy_name );
            int locus_index_replace = m_pGenes->GetLocusIndex( to_replace_name );
            m_AlleleIndexToCopy    = m_pGenes->GetAlleleIndex( to_copy_name );
            m_AlleleIndexToReplace = m_pGenes->GetAlleleIndex( to_replace_name );

            if( m_LocusIndex != locus_index_replace )
            {
                std::stringstream ss;
                ss << "The 'Allele_To_Copy' (='" << to_copy_name << "') and the 'Allele_To_Replace' (='" << to_replace_name << "')";
                ss << " are not from the same gene/locus.  They must effect the same gene/locus.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }

        if( ret )
        {
            // ---------------------------------------------------------------------------------------
            // --- The Copy_To_Likelihood (CTL) parameter must be configured after the Allele_To_Copy
            // --- and Alllele_To_Replace parameters are configured.  CTL needs this information when
            // --- verifying that the allele read in are valid - same locus as these other parameters.
            // ---------------------------------------------------------------------------------------
            initConfigComplexCollectionType( "Copy_To_Likelihood", &m_CopyToAlleleLikelihoods, AlleleDrive_Copy_To_Likelihood_DESC_TEXT );

            ret = JsonConfigurable::Configure( config );
            if( ret && !JsonConfigurable::_dryrun )
            {
                m_CopyToAlleleLikelihoods.CheckConfiguration();

                bool found = false;
                float total_prob = 0.0;
                for( int i = 0; i < m_CopyToAlleleLikelihoods.Size(); ++i )
                {
                    const CopyToAlleleLikelihood* p_ctl = m_CopyToAlleleLikelihoods[ i ];
                    uint8_t copy_locus_index = m_pGenes->GetLocusIndex( p_ctl->GetCopyToAlleleName() );
                    if( copy_locus_index != m_LocusIndex )
                    {
                        std::stringstream ss;
                        ss << "Invalid allele defined in '"<< m_CopyToAlleleLikelihoods.GetCollectionName() <<"'.\n";
                        ss << "The allele='" << p_ctl->GetCopyToAlleleName()<< "' is invalid with 'Allele_To_Copy'='" << to_copy_name << "'.\n";
                        ss << "The allele defined in 'Copy_To_Likelihood' must be of the same gene/locus as 'Allele_To_Copy' and 'Allele_To_Replace'.";
                        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                    }

                    if( p_ctl->GetCopyToAlleleName() == to_replace_name )
                    {
                        m_ProbabilityOfFailure = p_ctl->GetLikelihood();
                        found = true;
                    }
                    total_prob += p_ctl->GetLikelihood();
                }
                if( fabs( 1.0 - total_prob ) > FLT_EPSILON )
                {
                    std::stringstream ss;
                    ss << "Invalid 'Copy_To_Likelihood' probabilities for 'Allele_To_Copy'='" << to_copy_name << "'.\n";
                    ss << "The sum of the probabilities equals " << total_prob << " but they must sum to 1.0.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                if( !found )
                {
                    std::stringstream ss;
                    ss << "Missing allele in 'Copy_To_Likelihood'.\n";
                    ss << "The 'Allele_To_Replace'='" << to_replace_name << "' must have an entry in the 'Copy_To_Likelihood' list.\n";
                    ss << "The value represents failure to copy and can be zero.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
            }
        }
        return ret;
    }

    bool AlleleDriven::AreTheSame( const AlleleDriven& rThat ) const
    {
        if( this->m_LocusIndex           != rThat.m_LocusIndex           ) return false;
        if( this->m_AlleleIndexToCopy    != rThat.m_AlleleIndexToCopy    ) return false;
        if( this->m_AlleleIndexToReplace != rThat.m_AlleleIndexToReplace ) return false;

        if( this->m_CopyToAlleleLikelihoods.Size() != rThat.m_CopyToAlleleLikelihoods.Size() ) return false;

        for( int i = 0; i < this->m_CopyToAlleleLikelihoods.Size(); ++i )
        {
            const CopyToAlleleLikelihood* p_this_copy = this->m_CopyToAlleleLikelihoods[ i ];

            bool found = false;
            for( int j = 0; !found && (j < rThat.m_CopyToAlleleLikelihoods.Size()); ++j )
            {
                const CopyToAlleleLikelihood* p_that_copy = rThat.m_CopyToAlleleLikelihoods[ j ];
                if( p_this_copy->GetCopyToAlleleIndex() == p_that_copy->GetCopyToAlleleIndex() )
                {
                    found = true;
                    if( p_this_copy->GetLikelihood() != p_that_copy->GetLikelihood() )
                    {
                        return false;
                    }
                }
            }
            if( !found )
            {
                return false;
            }
        }
        return true;
    }

    uint8_t AlleleDriven::GetLocusIndex() const
    {
        return m_LocusIndex;
    }

    uint8_t AlleleDriven::GetAlleleIndexToCopy() const
    {
        return m_AlleleIndexToCopy;
    }

    uint8_t AlleleDriven::GetAlleleIndexToReplace() const
    {
        return m_AlleleIndexToReplace;
    }

    const CopyToAlleleLikelihoodCollection& AlleleDriven::GetCopyToAlleleLikelihoods() const
    {
        return m_CopyToAlleleLikelihoods;
    }

    void AlleleDriven::AddCopyToLikelhood( uint8_t alleleIndex, float likelihood )
    {
        std::string allele_name = m_pGenes->GetAlleleName( m_LocusIndex, alleleIndex );
        CopyToAlleleLikelihood* p_ctal = new CopyToAlleleLikelihood( m_pGenes,
                                                                     allele_name,
                                                                     alleleIndex,
                                                                     likelihood );
        m_CopyToAlleleLikelihoods.Add( p_ctal );
        if( m_AlleleIndexToReplace == alleleIndex )
        {
            m_ProbabilityOfFailure = likelihood;
        }
    }

    float AlleleDriven::GetProbabilityOfFailure() const
    {
        return m_ProbabilityOfFailure;
    }

    // ------------------------------------------------------------------------
    // --- AlleleDrivenCollection
    // ------------------------------------------------------------------------

    AlleleDrivenCollection::AlleleDrivenCollection( const VectorGeneCollection* pGenes )
        : JsonConfigurableCollection( "vector AlleleDriven" )
        , m_pGenes( pGenes )
    {
    }

    AlleleDrivenCollection::~AlleleDrivenCollection()
    {
    }

    void AlleleDrivenCollection::CheckConfiguration()
    {
        // do nothing
    }

    AlleleDriven* AlleleDrivenCollection::CreateObject()
    {
        return new AlleleDriven( m_pGenes );
    }

    // ------------------------------------------------------------------------
    // --- ShreddingAlleles
    // ------------------------------------------------------------------------


    ShreddingAlleles::ShreddingAlleles( const VectorGeneCollection* pGenes )
        : JsonConfigurable()
        , m_pGenes( pGenes )
        , m_LocusIndex( 0 )
        , m_AlleleIndexRequired( 0 )
        , m_AlleleIndexToShred( 0 )
        , m_AlleleIndexToShredTo( 0 )
        , m_ShreddingFraction( 1.0 )
        , m_SurvivingFraction( 0.0 )
    {
    }

    ShreddingAlleles::~ShreddingAlleles()
    {
    }

    bool ShreddingAlleles::Configure( const Configuration* config )
    {
        std::set<std::string> allowed_allele_names = m_pGenes->GetGenderGeneAlleleNames();

        jsonConfigurable::ConstrainedString required_name;
        required_name.constraint_param = &allowed_allele_names;
        required_name.constraints = m_pGenes->GENE_CONSTRAINTS;
        initConfigTypeMap( "Allele_Required", &required_name, SA_Allele_Required_DESC_TEXT );

        jsonConfigurable::ConstrainedString to_shred_name;
        to_shred_name.constraint_param = &allowed_allele_names;
        to_shred_name.constraints = m_pGenes->GENE_CONSTRAINTS;
        initConfigTypeMap( "Allele_To_Shred", &to_shred_name, SA_Allele_To_Shred_DESC_TEXT );

        jsonConfigurable::ConstrainedString to_shred_to_name;
        to_shred_to_name.constraint_param = &allowed_allele_names;
        to_shred_to_name.constraints = m_pGenes->GENE_CONSTRAINTS;
        initConfigTypeMap( "Allele_To_Shred_To", &to_shred_to_name, SA_Allele_To_Shread_To_DESC_TEXT );

        initConfigTypeMap( "Allele_Shredding_Fraction",             &m_ShreddingFraction, SA_Allele_Shredding_Fraction_DESC_TEXT,             0.0f, 1.0f, 1.0f );
        initConfigTypeMap( "Allele_To_Shred_To_Surviving_Fraction", &m_SurvivingFraction, SA_Allele_To_Shred_To_Surviving_Fraction_DESC_TEXT, 0.0f, 1.0f, 0.0f );

        bool ret = JsonConfigurable::Configure( config );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( (required_name == to_shred_name) || (required_name == to_shred_to_name) || (to_shred_name == to_shred_to_name) )
            {
                std::stringstream ss;
                ss << "Invalid Shredding_Alleles\n";
                ss << "The alleles used in 'Shredding_Alleles' must all be different.\n";
                ss << "The read values are:\n";
                ss << "'Allele_Required'    = '" << required_name    << "'\n";
                ss << "'Allele_To_Shred'    = '" << to_shred_name    << "'\n";
                ss << "'Allele_To_Shred_To' = '" << to_shred_to_name << "'\n";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            m_LocusIndex = m_pGenes->GetLocusIndex( required_name );
            release_assert( m_LocusIndex == VectorGenome::GENDER_LOCUS_INDEX );

            m_AlleleIndexRequired  = m_pGenes->GetAlleleIndex( required_name    );
            m_AlleleIndexToShred   = m_pGenes->GetAlleleIndex( to_shred_name    );
            m_AlleleIndexToShredTo = m_pGenes->GetAlleleIndex( to_shred_to_name );
        }
        return ret;
    }

    AlleleDriven* ShreddingAlleles::ConvertToAlleleDriven() const
    {
        float fraction_not_shredded = 1.0 - m_ShreddingFraction;

        AlleleDriven* p_ad = new AlleleDriven( m_pGenes,
                                               m_LocusIndex,
                                               m_AlleleIndexRequired,
                                               m_AlleleIndexToShred );

        p_ad->AddCopyToLikelhood( m_AlleleIndexRequired,  0.0 );
        p_ad->AddCopyToLikelhood( m_AlleleIndexToShred,   fraction_not_shredded );
        p_ad->AddCopyToLikelhood( m_AlleleIndexToShredTo, m_ShreddingFraction );

        return p_ad;
    }

    uint8_t ShreddingAlleles::GetLocusIndex() const
    {
        return m_LocusIndex;
    }

    uint8_t ShreddingAlleles::GetAlleleIndexRequired() const
    {
        return m_AlleleIndexRequired;
    }

    uint8_t ShreddingAlleles::GetAlleleIndexToShred() const
    {
        return m_AlleleIndexToShred;
    }

    uint8_t ShreddingAlleles::GetAlleleIndexToShredTo() const
    {
        return m_AlleleIndexToShredTo;
    }

    float ShreddingAlleles::GetShreddingSurvivingFraction() const
    {
        return m_SurvivingFraction;
    }

    // ------------------------------------------------------------------------
    // --- VectorGeneDriver
    // ------------------------------------------------------------------------

    VectorGeneDriver::VectorGeneDriver( const VectorGeneCollection* pGenes,
                                        VectorTraitModifiers* pTraitModifiers )
        : JsonConfigurable()
        , m_pGenes( pGenes )
        , m_pTraitModifiers( pTraitModifiers )
        , m_DriverType( VectorGeneDriverType::CLASSIC )
        , m_DriverLocusIndex(0)
        , m_DriverAlleleIndex(0)
        , m_AllelesDriven( pGenes )
        , m_AllelesDrivenByLocus()
    {
        m_AllelesDrivenByLocus.resize( m_pGenes->Size(), nullptr );
    }

    VectorGeneDriver::~VectorGeneDriver()
    {
    }

    bool VectorGeneDriver::Configure( const Configuration* config )
    {
        initConfig( "Driver_Type", m_DriverType, config, MetadataDescriptor::Enum( "Driver_Type", VGD_Driver_Type_DESC_TEXT, MDD_ENUM_ARGS( VectorGeneDriverType ) ) );

        const std::set<std::string>& allowed_allele_names = m_pGenes->GetDefinedAlleleNames();
        jsonConfigurable::ConstrainedString allele_name;
        allele_name.constraint_param = &allowed_allele_names;
        allele_name.constraints = m_pGenes->GENE_CONSTRAINTS;
        initConfigTypeMap( "Driving_Allele", &allele_name, VGD_Driving_Allele_DESC_TEXT );

        initConfigComplexCollectionType( "Alleles_Driven", &m_AllelesDriven, VGD_Alleles_Driven_DESC_TEXT, "Driver_Type", "CLASSIC,INTEGRAL_AUTONOMOUS,DAISY_CHAIN" );

        AlleleDriven driving_allele_params( m_pGenes );
        ShreddingAlleles shredding_alleles( m_pGenes );
        initConfigTypeMap( "Driving_Allele_Params", &driving_allele_params, VGD_Driving_Allele_Params_DESC_TEXT, "Driver_Type", "X_SHRED,Y_SHRED" );
        initConfigTypeMap( "Shredding_Alleles",     &shredding_alleles,     VGD_Shredding_Alleles_DESC_TEXT,     "Driver_Type", "X_SHRED,Y_SHRED" );

        bool ret = JsonConfigurable::Configure( config );
        if( ret && !JsonConfigurable::_dryrun )
        {
            m_DriverLocusIndex = m_pGenes->GetLocusIndex( allele_name );
            m_DriverAlleleIndex = m_pGenes->GetAlleleIndex( allele_name );

            // ------------------------------------------------------------------------------
            // --- We used to allow other loci to be effectors but are stopping that for now
            // ------------------------------------------------------------------------------
            if( (m_DriverType == VectorGeneDriverType::CLASSIC) && (m_AllelesDriven.Size() != 1) )
            {
                std::stringstream ss;
                ss << "Invalid Classic Drive\n";
                ss << "Classic drive with 'Driving_Allele' = '" << allele_name << "' must have exactly one entry in in 'Alleles_Driven'.\n";
                ss << "This entry must have 'Allele_To_Copy' equal to 'Driving_Allele'\n";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            else if( (m_DriverType == VectorGeneDriverType::X_SHRED) || (m_DriverType == VectorGeneDriverType::Y_SHRED) )
            {
                m_AllelesDriven.Add( new AlleleDriven( driving_allele_params ) );
                ConvertShreddingAlleles( shredding_alleles );
            }
            else if( m_DriverType == VectorGeneDriverType::DAISY_CHAIN )
            {
                for( int i = 0; i < m_AllelesDriven.Size(); ++i )
                {
                    AlleleDriven* p_allele_driven = m_AllelesDriven[ i ];
                    uint8_t locus_index = p_allele_driven->GetLocusIndex();
                    uint8_t allele_index = p_allele_driven->GetAlleleIndexToCopy();
                    if( (locus_index == m_DriverLocusIndex) && (allele_index == m_DriverAlleleIndex) )
                    {
                        const CopyToAlleleLikelihoodCollection& r_copy_to_collection = p_allele_driven->GetCopyToAlleleLikelihoods();
                        for( int j = 0; j < r_copy_to_collection.Size(); ++j )
                        {
                            uint8_t copy_to_allele_index = r_copy_to_collection[ j ]->GetCopyToAlleleIndex();
                            if( copy_to_allele_index == m_DriverAlleleIndex )
                            {
                                std::stringstream ss;
                                ss << "Invalid Daisy Chain Drive\n";
                                ss << "Daisy Chain drive with 'Driving_Allele' = '" << allele_name << "' cannot drive itself.\n";
                                ss << "This drive cannot have a 'Copy_To_Allele' equal to the 'Driving_Allele'\n";
                                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                            }
                        }
                    }
                }
            }

            for( int i  = 0; i < m_AllelesDriven.Size(); ++i )
            {
                AlleleDriven* p_allele_driven = m_AllelesDriven[i];
                uint8_t locus_index = p_allele_driven->GetLocusIndex();
                if( m_AllelesDrivenByLocus[ locus_index ] != nullptr )
                {
                    uint8_t to_copy_allele_index_b = m_AllelesDrivenByLocus[ locus_index ]->GetAlleleIndexToCopy();
                    uint8_t to_copy_allele_index_a = p_allele_driven->GetAlleleIndexToCopy();
                    std::string to_copy_name_a = m_pGenes->GetAlleleName( locus_index, to_copy_allele_index_a );
                    std::string to_copy_name_b = m_pGenes->GetAlleleName( locus_index, to_copy_allele_index_b );

                    std::stringstream ss;
                    ss << "Invalid 'Alleles_Driven' for gene driver with 'Driving_Allele'='" << allele_name <<"'.\n";
                    ss << "There are at least two alleles to drive that effect the same gene/locus.\n";
                    ss << "'Allele_To_Copy'='" << to_copy_name_a << "' and 'Allele_To_Copy'='" << to_copy_name_b << "' are from the same gene/locus.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                m_AllelesDrivenByLocus[ locus_index ] = p_allele_driven;
            }
            CheckAllelesDrivenDrivingAllele();
        }
        return ret;
    }

    void VectorGeneDriver::ConvertShreddingAlleles( const ShreddingAlleles& rShreddingAlleles )
    {
        const VectorGene* p_gender_gene = (*m_pGenes)[ rShreddingAlleles.GetLocusIndex() ];
        const VectorAllele* p_allele_required    = p_gender_gene->GetAllele( rShreddingAlleles.GetAlleleIndexRequired()  );
        const VectorAllele* p_allele_to_shred    = p_gender_gene->GetAllele( rShreddingAlleles.GetAlleleIndexToShred()   );
        const VectorAllele* p_allele_to_shred_to = p_gender_gene->GetAllele( rShreddingAlleles.GetAlleleIndexToShredTo() );

        if( ((m_DriverType == VectorGeneDriverType::X_SHRED) && p_allele_required->IsFemale()) ||
            ((m_DriverType == VectorGeneDriverType::Y_SHRED) && p_allele_required->IsMale()  ) )
        {
            std::string driver_name = VectorGeneDriverType::pairs::lookup_key( m_DriverType );
            std::string required_name = p_allele_required->GetName();
            std::string not_str = (m_DriverType == VectorGeneDriverType::X_SHRED) ? "" : "NOT ";
            std::stringstream ss;
            ss << "Invalid Shredding_Alleles\n";
            ss << "The 'Allele_Required' (='" << required_name << "') must be an allele that is " << not_str << "a Y-Chromosome when using '" << driver_name << "'.\n";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        if( ((m_DriverType == VectorGeneDriverType::X_SHRED) && p_allele_to_shred->IsMale()) ||
            ((m_DriverType == VectorGeneDriverType::Y_SHRED) && p_allele_to_shred->IsFemale()) )
        {
            std::string driver_name = VectorGeneDriverType::pairs::lookup_key( m_DriverType );
            std::string to_shred_name = p_allele_to_shred->GetName();
            std::string not_str = (m_DriverType == VectorGeneDriverType::X_SHRED) ? "NOT " : "";
            std::stringstream ss;
            ss << "Invalid Shredding_Alleles\n";
            ss << "The 'Allele_To_Shred' (='" << to_shred_name << "') must be an allele that is " << not_str << "a Y-Chromosome when using '" << driver_name << "'.\n";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        if( ((m_DriverType == VectorGeneDriverType::X_SHRED) && p_allele_to_shred_to->IsMale()) ||
            ((m_DriverType == VectorGeneDriverType::Y_SHRED) && p_allele_to_shred_to->IsFemale()) )
        {
            std::string driver_name = VectorGeneDriverType::pairs::lookup_key( m_DriverType );
            std::string to_shred_to_name = p_allele_to_shred_to->GetName();
            std::string not_str = (m_DriverType == VectorGeneDriverType::X_SHRED) ? "NOT " : "";
            std::stringstream ss;
            ss << "Invalid Shredding_Alleles\n";
            ss << "The 'Allele_To_Shred_To' (='" << to_shred_to_name << "') must be an allele that is " << not_str << "a Y-Chromosome when using '" << driver_name << "'.\n";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        m_AllelesDriven.Add( rShreddingAlleles.ConvertToAlleleDriven() );

        std::string to_shred_to_allele_name = m_pGenes->GetAlleleName( rShreddingAlleles.GetLocusIndex(),
                                                                       rShreddingAlleles.GetAlleleIndexToShredTo() );
        std::vector<std::string> combo;
        combo.push_back( to_shred_to_allele_name );
        combo.push_back( "*" );

        std::vector<std::vector<std::string>> combo_strings;
        combo_strings.push_back( combo );

        VectorGameteBitPair_t bit_mask = 0;
        std::vector<VectorGameteBitPair_t> possible_genomes;

        m_pGenes->ConvertAlleleCombinationsStrings( "Allele_Combinations",
                                                    combo_strings,
                                                    &bit_mask,
                                                    &possible_genomes );

        GeneToTraitModifier* p_gttm = new GeneToTraitModifier( VectorTrait::ADJUST_FERTILE_EGGS,
                                                               bit_mask,
                                                               possible_genomes,
                                                               std::vector<int64_t>(),
                                                               std::vector<int64_t>(),
                                                               rShreddingAlleles.GetShreddingSurvivingFraction() );
        m_pTraitModifiers->AddModifier( p_gttm );
    }

    void VectorGeneDriver::CheckAllelesDrivenDrivingAllele()
    {
        if( m_AllelesDrivenByLocus[ m_DriverLocusIndex ] == nullptr )
        {
            std::string driver_allele_name = m_pGenes->GetAlleleName( m_DriverLocusIndex, m_DriverAlleleIndex );
            std::stringstream ss;
            if( (m_DriverType == VectorGeneDriverType::X_SHRED) || (m_DriverType == VectorGeneDriverType::Y_SHRED) )
            {
                ss << "The gene driver with 'Driving_Allele'='" << driver_allele_name << "'\n";
                ss << "must have the 'Driving_Allele' equal to 'Allele_To_Copy' in the 'Driving_Allele_Params'.\n";
            }
            else
            {
                ss << "The gene driver with 'Driving_Allele'='" << driver_allele_name << "' must have exactly one entry in 'Alleles_Driven' for this allele.";
            }
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        if( m_AllelesDrivenByLocus[ m_DriverLocusIndex ]->GetAlleleIndexToCopy() != m_DriverAlleleIndex )
        {
            uint8_t to_copy_allele_index = m_AllelesDrivenByLocus[ m_DriverLocusIndex ]->GetAlleleIndexToCopy();
            std::string driver_allele_name = m_pGenes->GetAlleleName( m_DriverLocusIndex, m_DriverAlleleIndex );
            std::string to_copy_name = m_pGenes->GetAlleleName( m_DriverLocusIndex, to_copy_allele_index );
            std::stringstream ss;
            ss << "Invalid Gene Driver\n";
            ss << "The gene driver with 'Driving_Allele'='" << driver_allele_name << "' must have exactly\n";
            ss << "one entry in 'Alleles_Driven' where 'Allele_To_Copy' is the same as the 'Driving_Allele'.\n";
            ss << "The entry in 'Alleles_Driven' for the same locus as the 'Driving_Allele' has\n";
            ss << "'Allele_To_Copy'='" << to_copy_name << "'";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    bool VectorGeneDriver::CanBeDriven( const VectorGenome& rGenome ) const
    {
        std::pair<uint8_t, uint8_t> driver_indexes = rGenome.GetLocus( m_DriverLocusIndex );

        // --------------------------------------------------
        // --- Check if one of the gametes has the driver
        // --------------------------------------------------
        if( m_DriverType != VectorGeneDriverType::CLASSIC )
        {
            // ---------------------------------------------------------------------------
            // --- For the Intergral Autonomous driver, at least one of the gametes
            // --- must have the driver.  Alleles can still be driven if the driving
            // --- allele is in both gametes or even if the driving allele cannot replace
            // --- the allele in the other gamete.
            // ---------------------------------------------------------------------------
            return ( (driver_indexes.first  == m_DriverAlleleIndex) ||
                     (driver_indexes.second == m_DriverAlleleIndex) );
        }
        else
        {
            uint8_t non_driver_allele_index = driver_indexes.second;
            if( driver_indexes.first == driver_indexes.second )
            {
                // --------------------------------------------------------------------------
                // --- If the driving alleles are the same, then there is nothing to drive.
                // --- If the alleles at this locus are not drivers, then no driving is done.
                // --------------------------------------------------------------------------
                return false;
            }
            else if( driver_indexes.first == m_DriverAlleleIndex )
            {
                non_driver_allele_index = driver_indexes.second;
            }
            else if( driver_indexes.second == m_DriverAlleleIndex )
            {
                non_driver_allele_index = driver_indexes.first;
            }
            else
            {
                // ----------------------------------------------------------------
                // --- if neither alleles at this locus are the driver, then false
                // ----------------------------------------------------------------
                return false;
            }

            // --------------------------------------------------------------------------
            // --- The driver can only drive if the one gamete has the driving
            // --- allele and the other has a specific allele to be replaced.
            // --------------------------------------------------------------------------
            uint8_t allele_index_to_replace = m_AllelesDrivenByLocus[ m_DriverLocusIndex ]->GetAlleleIndexToReplace();
            return ( allele_index_to_replace == non_driver_allele_index );
        }
    }

    GenomeProbPairVector_t VectorGeneDriver::DriveGenes( const VectorGenome& rGenome ) const
    {
        // -------------------------------------------
        // --- In the pair, first = mom, second = dad
        // -------------------------------------------
        std::pair<uint8_t,uint8_t> driver_indexes = rGenome.GetLocus( m_DriverLocusIndex );

        VectorGenomeGameteIndex::Enum gamete_index_from = VectorGenomeGameteIndex::GAMETE_INDEX_MOM;
        VectorGenomeGameteIndex::Enum gamete_index_to   = VectorGenomeGameteIndex::GAMETE_INDEX_DAD;
        if( driver_indexes.second == m_DriverAlleleIndex )
        {
            gamete_index_from = VectorGenomeGameteIndex::GAMETE_INDEX_DAD;
            gamete_index_to   = VectorGenomeGameteIndex::GAMETE_INDEX_MOM;
        }

        GenomeProbPairVector_t gpp_list;
        if( m_DriverType == VectorGeneDriverType::CLASSIC )
        {
            gpp_list = ClassicDrive( gamete_index_from, gamete_index_to, rGenome );
        }
        else if( (m_DriverType == VectorGeneDriverType::INTEGRAL_AUTONOMOUS) ||
                 (m_DriverType == VectorGeneDriverType::DAISY_CHAIN        ) ||
                 (m_DriverType == VectorGeneDriverType::X_SHRED            ) ||
                 (m_DriverType == VectorGeneDriverType::Y_SHRED            ) )
        {
            gpp_list = AutonomousDrive( rGenome );
        }
        else
        {
            throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__,
                                                     "m_DriverType",
                                                     m_DriverType,
                                                     VectorGeneDriverType::pairs::lookup_key(m_DriverType));
        }
        return gpp_list;
    }

    GenomeProbPairVector_t VectorGeneDriver::ClassicDrive( VectorGenomeGameteIndex::Enum fromGameteIndex,
                                                           VectorGenomeGameteIndex::Enum toGameteIndex,
                                                           const VectorGenome& rGenome ) const
    {
        AlleleDriven* p_driver_ad = m_AllelesDrivenByLocus[ m_DriverLocusIndex ];
        float failure_prob = p_driver_ad->GetProbabilityOfFailure();
        GenomeProbPair gpp_not_driven( rGenome, failure_prob );

        VectorGamete from_gamete = gpp_not_driven.genome.GetGamete( fromGameteIndex );
        VectorGamete to_gamete   = gpp_not_driven.genome.GetGamete( toGameteIndex );

        GenomeProbPairVector_t gpp_list;
        const CopyToAlleleLikelihoodCollection& r_likelihoods = p_driver_ad->GetCopyToAlleleLikelihoods();
        for( int i = 0; i < r_likelihoods.Size(); ++i )
        {
            const CopyToAlleleLikelihood* p_ctl = r_likelihoods[ i ];
            if( p_ctl->GetCopyToAlleleIndex() == p_driver_ad->GetAlleleIndexToReplace() ) continue; //failure to copy

            GenomeProbPair gpp_driven( rGenome, p_ctl->GetLikelihood() );

            gpp_driven.genome.GetGamete( toGameteIndex ).SetLocus( p_driver_ad->GetLocusIndex(),
                                                                   p_ctl->GetCopyToAlleleIndex() );
            gpp_list.push_back( gpp_driven );
        }

        gpp_list.push_back( gpp_not_driven );
        return gpp_list;
    }

    // ------------------------------------------------------------------------------
    // In DanB's understanding, the idea of "germline gene drives" is built on 
    // the idea that allele in the entities DNA will have blueprints for producing
    // Cas9 and guide RNA (gRNA).  During Interphase (which happens right before
    // meiosis) when the chromosomes are still paired up (one from each parent),
    // the Cas9 and gRNA are produced.  These guys will temporarily team up and
    // look for the targeted DNA strand, usually a wild-type allele.  When found,
    // the Cas9 will cut out the targeted allele and then let 
    // Homology Directed Repair (HDR) do the work of copying the desired allele
    // from the sister stand/chromosome into the "hole" created by the Cas9.  
    // If the Cas9+gRNA cannot find the targeted allele, they break up and the
    // Cas9 can look for another gRNA.  [Shirley -It's true-ish in that the Cas9+gRNA 
    // could break up at any time (though they have a really high affinity for each
    // other, so not too often) such that the free Cas9 can then run into and bind
    // another gRNA, but it's not really on purpose if the targeted allele can't
    // be found. It just happens randomly.]
    //
    // There can be problems during this process.  The Cas9+gRNA might not find
    // the targeted allele.  There could be problems during cutting that cause a
    // mutation to occur.  There can be problems with the copying.  One type of
    // mutation can come from Non-Homologous End Joining (NHEJ) that happens when
    // you have double stranded breaks.  I don't know how we get those exactly,
    // but the main idea is that you get mutations and sometimes these mutations
    // can make the cell non-viable while others can develop a "resistance" to
    // the drive.  This resistance is the formation of a valid allele that the
    // drive's gRNA doesn't recognize.  This means no cutting occurs which means
    // no copying occurs and no propagation of the  desired allele.
    //
    // In the Classic gene drive case, the DNA has the Cas9, gRNA, and the desired
    // allele (with some special phenotypic property) combined together.
    // During Interphase, if the Cas9+gRNA finds the targeted allele, it will cut
    // it out and copy over the blueprints for the Cas9, gRNA, and desired allele.
    // 
    // In contrast, the integral gene drive is based on the idea of having a driver
    // that has the Cas9+gRNA that knows how to find its self plus an effector, our
    // desired allele, that has its own gRNA. The effector allele doesn't need to be
    // bundled with the driving allele.  The Cas9 produced by the driver can also
    // team up with the gRNA produced by the effector so it can cut out the targeted
    // allele where we want the effector to go.
    //
    // In the case where you have more than one driver, a Cas9 and gRNA are being
    // produced for each driver.  If you have an effector, it is also producing gRNA.
    // We assume that a single driver produces an "equal" number of Cas9 and gRNA.
    // The presence of more drivers should not impact the likelihood of cutting the
    // targeted allele because they are all pairing up evenly.  However, the extra
    // Cas9 from the multiple drivers can impact the likelihood of cutting the
    // effector that is producing gRNA but not Cas9.  In the single driver case,
    // there is one set of Cas9 and two sets of gRNA.  In the two driver case,
    // there are two sets of Cas9 and three sets of gRNA, hence, there is a greater
    // likelihood of a Cas9 pairing up with the gRNA from the effector.
    // ------------------------------------------------------------------------------

    GenomeProbPairVector_t VectorGeneDriver::AutonomousDrive( const VectorGenome& rGenome ) const
    {
        VectorGenome copy = rGenome;
        VectorGamete gamete_mom = copy.GetGamete( VectorGenomeGameteIndex::GAMETE_INDEX_MOM );
        VectorGamete gamete_dad = copy.GetGamete( VectorGenomeGameteIndex::GAMETE_INDEX_DAD );

        bool drive_is_present = (gamete_mom.GetLocus( m_DriverLocusIndex ) == m_DriverAlleleIndex)
                             || (gamete_dad.GetLocus( m_DriverLocusIndex ) == m_DriverAlleleIndex);

        release_assert( drive_is_present );

        GenomeProbPairVector_t gpp_list;
        gpp_list.push_back( GenomeProbPair( rGenome, 1.0 ) );
        for( uint8_t locus_index = 0; locus_index < m_AllelesDrivenByLocus.size(); ++locus_index )
        {
            AlleleDriven* p_ad = m_AllelesDrivenByLocus[ locus_index ];
            if( p_ad == nullptr ) continue;

            VectorGenomeGameteIndex::Enum to_gamete_index= VectorGenomeGameteIndex::GAMETE_INDEX_DAD;
            if( (gamete_mom.GetLocus( locus_index ) == p_ad->GetAlleleIndexToCopy()   ) &&
                (gamete_dad.GetLocus( locus_index ) == p_ad->GetAlleleIndexToReplace()) )
            {
                to_gamete_index = VectorGenomeGameteIndex::GAMETE_INDEX_DAD;
            }
            else if( (gamete_dad.GetLocus( locus_index ) == p_ad->GetAlleleIndexToCopy()   ) &&
                     (gamete_mom.GetLocus( locus_index ) == p_ad->GetAlleleIndexToReplace()) )
            {
                to_gamete_index = VectorGenomeGameteIndex::GAMETE_INDEX_MOM;
            }
            else
            {
                continue;
            }

            const CopyToAlleleLikelihoodCollection& r_ad_likelihoods = p_ad->GetCopyToAlleleLikelihoods();

            GenomeProbPairVector_t tmp_gpp_list = gpp_list;
            gpp_list.clear();
            for( auto& r_gpp : tmp_gpp_list )
            {
                for( int i = 0; i < r_ad_likelihoods.Size(); ++i )
                {
                    const CopyToAlleleLikelihood* p_ctl = r_ad_likelihoods[ i ];
                    if( p_ctl->GetLikelihood() == 0.0 ) continue;
                    GenomeProbPair gpp = r_gpp;
                    gpp.genome.GetGamete( to_gamete_index ).SetLocus( locus_index, p_ctl->GetCopyToAlleleIndex() );
                    gpp.prob *= p_ctl->GetLikelihood();
                    gpp_list.push_back( gpp );
                }
            }
        }
        return gpp_list;
    }

    VectorGeneDriverType::Enum VectorGeneDriver::GetDriverType() const
    {
        return m_DriverType;
    }

    uint8_t VectorGeneDriver::GetDriverLocusIndex() const
    {
        return m_DriverLocusIndex;
    }

    uint8_t VectorGeneDriver::GetDriverAlleleIndex() const
    {
        return m_DriverAlleleIndex;
    }

    int VectorGeneDriver::GetNumLociDriven() const
    {
        return m_AllelesDriven.Size();
    }

    int VectorGeneDriver::GetNumAlleleToCopy( uint8_t locusIndex ) const
    {
        const AlleleDriven* p_ad = m_AllelesDrivenByLocus[ locusIndex ];
        int num = 0;
        if( p_ad != nullptr )
        {
            num = p_ad->GetCopyToAlleleLikelihoods().Size();
        }
        return num;
    }

    const AlleleDriven* VectorGeneDriver::GetAlleleDriven( uint8_t locusIndex ) const
    {
        return m_AllelesDrivenByLocus[ locusIndex ];
    }

    // ------------------------------------------------------------------------
    // --- VectorGeneDriverCollection
    // ------------------------------------------------------------------------

    VectorGeneDriverCollection::VectorGeneDriverCollection( const VectorGeneCollection* pGenes,
                                                            VectorTraitModifiers* pTraitModifiers )
        : JsonConfigurableCollection( "vector VectorGeneDriver" )
        , m_pGenes( pGenes )
        , m_pTraitModifiers( pTraitModifiers )
    {
    }

    VectorGeneDriverCollection::~VectorGeneDriverCollection()
    {
    }

    void VectorGeneDriverCollection::CheckConfiguration()
    {
        for( int i = 0; i < m_Collection.size(); ++i )
        {
            VectorGeneDriver* p_vgd_i = m_Collection[i];
            for( int j = i+1; j < m_Collection.size(); ++j )
            {
                VectorGeneDriver* p_vgd_j = m_Collection[ j ];
                CheckDriverOverlap( p_vgd_i, p_vgd_j );
           }
        }
    }

    bool VectorGeneDriverCollection::DoAllelesDrivenHaveSameLocus( const VectorGeneDriver* pVgdA, const VectorGeneDriver* pVgdB ) const
    {
        for( uint8_t locus_index = 0; locus_index < m_pGenes->Size(); ++locus_index )
        {
            if( (pVgdA->GetNumAlleleToCopy( locus_index ) > 0) && (pVgdB->GetNumAlleleToCopy( locus_index ) > 0) )
            {
                return true;
            }
        }
        return false;
    }

    bool VectorGeneDriverCollection::AreAllelesDrivenTheSame( const VectorGeneDriver* pVgdA, const VectorGeneDriver* pVgdB ) const
    {
        for( uint8_t locus_index = 0; locus_index < m_pGenes->Size(); ++locus_index )
        {
            // --------------------------
            // --- Ignore the driver loci
            // --------------------------
            if( (locus_index == pVgdA->GetDriverLocusIndex()) || (locus_index == pVgdB->GetDriverLocusIndex()) ) continue;

            const AlleleDriven* p_alleles_a = pVgdA->GetAlleleDriven( locus_index );
            const AlleleDriven* p_alleles_b = pVgdB->GetAlleleDriven( locus_index );

            if( (p_alleles_a != nullptr) && (p_alleles_b != nullptr) )
            {
                // -------------------------------------------------------
                // --- Check that this effector is the same for this locus
                // -------------------------------------------------------
                return p_alleles_a->AreTheSame( *p_alleles_b );
            }
            else if( ((p_alleles_a == nullptr) && (p_alleles_b != nullptr)) ||
                     ((p_alleles_a != nullptr) && (p_alleles_b == nullptr)) )
            {
                // ------------------------------------------------------
                // --- Each non-driver loci must have the same effectors
                // ------------------------------------------------------
                return false;
            }
        }
        return false;
    }

    void VectorGeneDriverCollection::CheckDriverOverlap( const VectorGeneDriver* pVgdA, const VectorGeneDriver* pVgdB ) const
    {
        // -------------------------------
        // --- Cannot mix driver types
        // -------------------------------
        if( pVgdA->GetDriverType() != pVgdB->GetDriverType() )
        {
            std::string driver_allele_name_a = m_pGenes->GetAlleleName( pVgdA->GetDriverLocusIndex(), pVgdA->GetDriverAlleleIndex() );
            std::string driver_allele_name_b = m_pGenes->GetAlleleName( pVgdB->GetDriverLocusIndex(), pVgdB->GetDriverAlleleIndex() );
            std::string driver_type_a = VectorGeneDriverType::pairs::lookup_key( pVgdA->GetDriverType() );
            std::string driver_type_b = VectorGeneDriverType::pairs::lookup_key( pVgdB->GetDriverType() );
            std::stringstream ss;
            ss << "Invalid Gene Drivers\n";
            ss << "Driver with 'Driving_Allele' '" << driver_allele_name_a << "' has 'Driver_Type' = '" << driver_type_a << "'.\n";
            ss << "Driver with 'Driving_Allele' '" << driver_allele_name_b << "' has 'Driver_Type' = '" << driver_type_b << "'.\n";
            ss << "One cannot mix driver types.\n";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        // --------------------------
        // --- Check for same driver
        // --------------------------
        if( (pVgdA->GetDriverLocusIndex() == pVgdB->GetDriverLocusIndex()) &&
            (pVgdA->GetDriverAlleleIndex() == pVgdB->GetDriverAlleleIndex()) )
        {
            std::string driver_allele_name = m_pGenes->GetAlleleName( pVgdA->GetDriverLocusIndex(), pVgdA->GetDriverAlleleIndex() );
            std::stringstream ss;
            ss << "Invalid Gene Driver Overlap\n";
            ss << "There are at least two drivers with 'Driving_Allele'='" << driver_allele_name << "'.\n";
            ss << "There can be only one.\n";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        // -------------------------------
        // --- Check for circular drivers
        // -------------------------------
        if( pVgdA->GetDriverLocusIndex() == pVgdB->GetDriverLocusIndex() )
        {
            const AlleleDriven* p_ad_a = pVgdA->GetAlleleDriven( pVgdA->GetDriverLocusIndex() );
            const AlleleDriven* p_ad_b = pVgdB->GetAlleleDriven( pVgdB->GetDriverLocusIndex() );
            if( (p_ad_a->GetAlleleIndexToCopy() == p_ad_b->GetAlleleIndexToReplace()) &&
                (p_ad_b->GetAlleleIndexToCopy() == p_ad_a->GetAlleleIndexToReplace()) )
            {
                std::string driver_allele_name_a = m_pGenes->GetAlleleName( pVgdA->GetDriverLocusIndex(), pVgdA->GetDriverAlleleIndex() );
                std::string driver_allele_name_b = m_pGenes->GetAlleleName( pVgdB->GetDriverLocusIndex(), pVgdB->GetDriverAlleleIndex() );
                std::stringstream ss;
                ss << "Invalid Gene Drivers\n";
                ss << "Drivers with 'Driving_Allele' '" << driver_allele_name_a << "' and '" << driver_allele_name_b << "' are circular.\n";
                ss << "You cannot have drivers that attempt to replace each other.\n";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }

        // -----------------------------------------------------------------------
        // --- Classic drivers allowed only the locus that the driver is on and
        // --- the mutation of one can be a driver that replaces the other driver.
        // --- It is a sort of circular driver going through a mutation.
        // -----------------------------------------------------------------------
        if( (pVgdA->GetDriverType() == VectorGeneDriverType::CLASSIC) &&
            (pVgdB->GetDriverType() == VectorGeneDriverType::CLASSIC) )
        {
            bool is_circular = false;
            const AlleleDriven* p_ad_a = pVgdA->GetAlleleDriven( pVgdA->GetDriverLocusIndex() );
            const AlleleDriven* p_ad_b = pVgdB->GetAlleleDriven( pVgdB->GetDriverLocusIndex() );
            if( p_ad_a->GetAlleleIndexToCopy() == p_ad_b->GetAlleleIndexToReplace() )
            {
                const CopyToAlleleLikelihoodCollection& r_ctal_list = p_ad_a->GetCopyToAlleleLikelihoods();
                for( int i = 0; i < r_ctal_list.Size(); ++i )
                {
                    const CopyToAlleleLikelihood& r_ctl = *r_ctal_list[ i ];
                    if( r_ctl.GetCopyToAlleleIndex() == p_ad_b->GetAlleleIndexToCopy() )
                    {
                        std::string driver_allele_name_a = m_pGenes->GetAlleleName( pVgdA->GetDriverLocusIndex(), pVgdA->GetDriverAlleleIndex() );
                        std::string driver_allele_name_b = m_pGenes->GetAlleleName( pVgdB->GetDriverLocusIndex(), pVgdB->GetDriverAlleleIndex() );
                        std::string effector_copy_a = m_pGenes->GetAlleleName( p_ad_a->GetLocusIndex(), p_ad_a->GetAlleleIndexToCopy() );
                        std::string effector_copy_b = m_pGenes->GetAlleleName( p_ad_b->GetLocusIndex(), p_ad_b->GetAlleleIndexToCopy() );
                        std::string effector_replace_b = m_pGenes->GetAlleleName( p_ad_b->GetLocusIndex(), p_ad_b->GetAlleleIndexToReplace() );
                        std::stringstream ss;
                        ss << "Invalid Gene Drivers\n";
                        ss << "Driver with 'Driving_Allele' '" << driver_allele_name_a << "' has '"<< r_ctal_list.GetCollectionName() <<"' = '" << effector_copy_b << "'.\n";
                        ss << "Driver with 'Driving_Allele' '" << driver_allele_name_b << "' has 'Allele_To_Replace' = '" << effector_replace_b << "'.\n";
                        ss << "This will cause a circular issue that the model does not support at this time.\n";
                        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                    }
                }
            }
            else if( p_ad_a->GetAlleleIndexToReplace() == p_ad_b->GetAlleleIndexToCopy() )
            {
                const CopyToAlleleLikelihoodCollection& r_ctal_list = p_ad_b->GetCopyToAlleleLikelihoods();
                for( int i = 0; i < r_ctal_list.Size(); ++i )
                {
                    const CopyToAlleleLikelihood& r_ctl = *r_ctal_list[ i ];
                    if( r_ctl.GetCopyToAlleleIndex() == p_ad_a->GetAlleleIndexToCopy() )
                    {
                        std::string driver_allele_name_a = m_pGenes->GetAlleleName( pVgdA->GetDriverLocusIndex(), pVgdA->GetDriverAlleleIndex() );
                        std::string driver_allele_name_b = m_pGenes->GetAlleleName( pVgdB->GetDriverLocusIndex(), pVgdB->GetDriverAlleleIndex() );
                        std::string effector_copy_a = m_pGenes->GetAlleleName( p_ad_a->GetLocusIndex(), p_ad_a->GetAlleleIndexToCopy() );
                        std::string effector_copy_b = m_pGenes->GetAlleleName( p_ad_b->GetLocusIndex(), p_ad_b->GetAlleleIndexToCopy() );
                        std::string effector_replace_a = m_pGenes->GetAlleleName( p_ad_a->GetLocusIndex(), p_ad_a->GetAlleleIndexToReplace() );
                        std::stringstream ss;
                        ss << "Invalid Gene Drivers\n";
                        ss << "Driver with 'Driving_Allele' '" << driver_allele_name_a << "' has 'Allele_To_Replace' = '" << effector_replace_a << "'.\n";
                        ss << "Driver with 'Driving_Allele' '" << driver_allele_name_b << "' has '" << r_ctal_list.GetCollectionName() << "' = '" << effector_copy_a << "'.\n";
                        ss << "This will cause a circular issue that the model does not support at this time.\n";
                        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                    }
                }
            }
        }

        // ----------------------------------
        // --- Check for circular effectors
        // ----------------------------------
        for( uint8_t locus_index = 0; locus_index < m_pGenes->Size(); ++locus_index )
        {
            const AlleleDriven* p_ad_a = pVgdA->GetAlleleDriven( locus_index );
            const AlleleDriven* p_ad_b = pVgdB->GetAlleleDriven( locus_index );
            if( (p_ad_a == nullptr) || (p_ad_b == nullptr) ) continue;

            if( (p_ad_a->GetAlleleIndexToCopy() == p_ad_b->GetAlleleIndexToReplace()) &&
                (p_ad_b->GetAlleleIndexToCopy() == p_ad_a->GetAlleleIndexToReplace()) )
            {
                std::string driver_allele_name_a = m_pGenes->GetAlleleName( pVgdA->GetDriverLocusIndex(), pVgdA->GetDriverAlleleIndex() );
                std::string driver_allele_name_b = m_pGenes->GetAlleleName( pVgdB->GetDriverLocusIndex(), pVgdB->GetDriverAlleleIndex() );
                std::string effector_allele_name_a = m_pGenes->GetAlleleName( locus_index, p_ad_a->GetAlleleIndexToCopy() );
                std::string effector_allele_name_b = m_pGenes->GetAlleleName( locus_index, p_ad_b->GetAlleleIndexToCopy() );
                std::stringstream ss;
                ss << "Invalid Gene Drivers\n";
                ss << "Driver with 'Driving_Allele' '" << driver_allele_name_a << "' and effector '" << effector_allele_name_a << "' and\n";
                ss << "driver with 'Driving_Allele' '" << driver_allele_name_b << "' and effector '" << effector_allele_name_b << "'\n";
                ss << "are circular.  You cannot have effectors that attempt to replace each other.\n";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }

        if( (pVgdA->GetDriverType() == pVgdB->GetDriverType()) &&
            (pVgdA->GetDriverType() != VectorGeneDriverType::CLASSIC) &&
            (pVgdA->GetDriverType() != VectorGeneDriverType::DAISY_CHAIN) )
        {
            // --------------------------------------------------------------------------------------
            // --- The Integral driver produces the same Cas9 regardless of the dirver producing it.
            // --- Hence, the Alleles_Driven (minus the driver) need to be the same - same alleles
            // --- and same probabilities.
            // --------------------------------------------------------------------------------------
            if( (pVgdA->GetNumAlleleToCopy( pVgdB->GetDriverLocusIndex() ) > 0) ||
                (pVgdB->GetNumAlleleToCopy( pVgdA->GetDriverLocusIndex() ) > 0) )
            {
                std::string driver_type_name = VectorGeneDriverType::pairs::lookup_key( pVgdA->GetDriverType() );
                std::string driver_allele_name_a = m_pGenes->GetAlleleName( pVgdA->GetDriverLocusIndex(), pVgdA->GetDriverAlleleIndex() );
                std::string driver_allele_name_b = m_pGenes->GetAlleleName( pVgdB->GetDriverLocusIndex(), pVgdB->GetDriverAlleleIndex() );
                if( pVgdA->GetDriverType() == VectorGeneDriverType::INTEGRAL_AUTONOMOUS )
                {
                    std::stringstream ss;
                    ss << "Invalid Gene Driver Overlap\n";
                    ss << "Drivers with 'Driving_Allele' '" << driver_allele_name_a << "' and '" << driver_allele_name_b << "' have\n";
                    ss << "'Alleles_Driven' that operate on the locus of the other driver.\n";
                    ss << "'" << driver_type_name << "' drivers can be used together but their 'Alleles_Driven'\n";
                    ss << "must have the same alleles, probabilities, AND cannot include the locus of another driver.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                else
                {
                    release_assert( (pVgdA->GetDriverType() == VectorGeneDriverType::X_SHRED) ||
                                    (pVgdA->GetDriverType() != VectorGeneDriverType::Y_SHRED) );

                    std::stringstream ss;
                    ss << "Invalid Gene Driver Overlap\n";
                    ss << "Drivers with 'Driving_Allele' '" << driver_allele_name_a << "' and '" << driver_allele_name_b << "' have\n";
                    ss << "'Driving_Allele_Params' that operate on the same locus.\n";
                    ss << "'" << driver_type_name << "' drivers can be used together but their 'Driving_Allele_Params'\n";
                    ss << "must be alleles from a different gene/locus.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
            }
            else if( !AreAllelesDrivenTheSame( pVgdA, pVgdB ) )
            {
                // ---------------------------------------------------------------------------
                // --- Integral drivers must have the same effector alleles and probabilities.
                // ---------------------------------------------------------------------------
                std::string driver_type_name = VectorGeneDriverType::pairs::lookup_key( pVgdA->GetDriverType() );
                std::string driver_allele_name_a = m_pGenes->GetAlleleName( pVgdA->GetDriverLocusIndex(), pVgdA->GetDriverAlleleIndex() );
                std::string driver_allele_name_b = m_pGenes->GetAlleleName( pVgdB->GetDriverLocusIndex(), pVgdB->GetDriverAlleleIndex() );
                if( pVgdA->GetDriverType() == VectorGeneDriverType::INTEGRAL_AUTONOMOUS )
                {
                    std::stringstream ss;
                    ss << "Invalid Gene Driver Overlap\n";
                    ss << "Drivers with 'Driving_Allele' '" << driver_allele_name_a << "' and '" << driver_allele_name_b << "' have ";
                    ss << "effectors in 'Alleles_Driven' that are not the same.\n";
                    ss << "'" << driver_type_name << "' drivers can be used together but the effectors in 'Alleles_Driven'\n";
                    ss << "must have the same alleles AND probabilities.  Only the drivers can be different.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                else
                {
                    release_assert( (pVgdA->GetDriverType() == VectorGeneDriverType::X_SHRED) ||
                                    (pVgdA->GetDriverType() != VectorGeneDriverType::Y_SHRED) );

                    std::stringstream ss;
                    ss << "Invalid Gene Driver Overlap\n";
                    ss << "Drivers with 'Driving_Allele' '" << driver_allele_name_a << "' and '" << driver_allele_name_b << "' have ";
                    ss << "alleles in 'Shredding_Alleles' that are not the same.\n";
                    ss << "'" << driver_type_name << "' drivers can be used together but the alleles in 'Shredding_Alleles'\n";
                    ss << "must be the same AND have the same 'Allele_Shredding_Fraction'.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
            }
        }
    }

    GenomeProbPairVector_t VectorGeneDriverCollection::DriveGenes( const VectorGenome& rGenome ) const
    {
        GenomeProbPairVector_t gpp_list;

        GenomeProbPair init_gpp( rGenome, 1.0 );
        gpp_list.push_back( init_gpp );

        for( auto p_vgd : m_Collection )
        {
            if( !(p_vgd->CanBeDriven( rGenome )) ) continue;

            GenomeProbPairVector_t tmp_gpp_list = gpp_list;
            gpp_list.clear();
            for( auto tmp_gpp : tmp_gpp_list )
            {
                if( tmp_gpp.prob == 0.0 ) continue;

                if( p_vgd->CanBeDriven( tmp_gpp.genome ) )
                {
                    GenomeProbPairVector_t drive_gpp_list = p_vgd->DriveGenes( tmp_gpp.genome );
                    for( auto driven_gpp : drive_gpp_list )
                    {
                        // propagate the previous probability
                        driven_gpp.prob *= tmp_gpp.prob;

                        // Add to the list avoiding duplicates
                        bool found = false;
                        for( int i = 0; !found && (i < gpp_list.size()); ++i )
                        {
                            if( gpp_list[ i ].genome == driven_gpp.genome )
                            {
                                gpp_list[ i ].prob += driven_gpp.prob;
                                found = true;
                            }
                        }
                        if( !found )
                        {
                            gpp_list.push_back( driven_gpp );
                        }
                    }
                }
            }
            //printf( "++++++++++++++++++++++++++\n" );
            //for( int i = 0; i < gpp_list.size(); ++i )
            //{
            //    GenomeProbPair gpp = gpp_list[ i ];
            //    printf( "%s[ %d ] = %10.7f\n", m_pGenes->GetGenomeName( gpp.genome ).c_str(), i, gpp.prob );
            //}
            //printf( "++++++++++++++++++++++++++\n" );
        }
        //printf( "=====================\n" );
        //for( int i = 0; i < gpp_list.size(); ++i )
        //{
        //    GenomeProbPair gpp = gpp_list[ i ];
        //    printf( "%s[ %d ] = %10.7f\n", m_pGenes->GetGenomeName( gpp.genome ).c_str(), i, gpp.prob );
        //}
        //printf( "=====================\n" );

        return gpp_list;
    }

    bool VectorGeneDriverCollection::HasDriverAndHeterozygous( const VectorGenome& rGenome ) const
    {
        for( auto p_vgd : m_Collection )
        {
            uint8_t driver_locus_index = p_vgd->GetDriverLocusIndex();
            uint8_t driver_allele_index = p_vgd->GetDriverAlleleIndex();

            std::pair<uint8_t,uint8_t> allele_at_locus = rGenome.GetLocus( driver_locus_index );

            bool is_heterozygous = (allele_at_locus.first != allele_at_locus.second);
            bool has_driver = (allele_at_locus.first  == driver_allele_index) ||
                              (allele_at_locus.second == driver_allele_index);

            if( is_heterozygous && has_driver )
            {
                return true;
            }
        }

        return false;
    }

    VectorGeneDriver* VectorGeneDriverCollection::CreateObject()
    {
        return new VectorGeneDriver( m_pGenes, m_pTraitModifiers );
    }
}