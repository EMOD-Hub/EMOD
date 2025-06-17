#include "stdafx.h"
#include "VectorGeneDriver.h"
#include "VectorGene.h"
#include "VectorTraitModifiers.h"
#include "VectorGeneDriver.h"
#include "VectorGenome.h"
#include "VectorMaternalDeposition.h"
#include "Exceptions.h"
#include "Log.h"
#include "Debug.h"
#include "IdmString.h"

SETUP_LOGGING( "VectorMaternalDeposition" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- CutToAlleleLikelihood
    // ------------------------------------------------------------------------

    CutToAlleleLikelihood::CutToAlleleLikelihood( const VectorGeneCollection* pGenes )
        : CopyToAlleleLikelihood( pGenes )
    {
    }

    CutToAlleleLikelihood::~CutToAlleleLikelihood()
    {
    }

    bool CutToAlleleLikelihood::Configure( const Configuration* config )
    {
        std::set<std::string> allowed_allele_names = m_pGenes->GetDefinedAlleleNames();

        jsonConfigurable::ConstrainedString name;
        name.constraint_param = &allowed_allele_names;
        name.constraints = "Vector_Species_Params[x].Genes";
        initConfigTypeMap( "Cut_To_Allele", &name, MD_Cut_To_Allele_DESC_TEXT );
        initConfigTypeMap( "Likelihood", &m_Prob, MD_Cut_Likelihood_DESC_TEXT, 0.0f, 1.0f, 0.0f );

        bool is_configured = JsonConfigurable::Configure( config );
        if( is_configured && !JsonConfigurable::_dryrun )
        {
            m_CopyToAlleleName = name;
            m_CopyToAlleleIndex = m_pGenes->GetAlleleIndex( m_CopyToAlleleName );
        }

        return is_configured;
    }



    // ------------------------------------------------------------------------
    // --- CutToAlleleLikelihoodCollection
    // ------------------------------------------------------------------------

    CutToAlleleLikelihoodCollection::CutToAlleleLikelihoodCollection( const VectorGeneCollection* pGenes, 
                                                                      std::string collectionName )
        : CopyToAlleleLikelihoodCollection(pGenes)
    {
        m_IdmTypeName = collectionName;
    }

    CutToAlleleLikelihoodCollection::~CutToAlleleLikelihoodCollection()
    {
    }

    std::string CutToAlleleLikelihoodCollection::GetCollectionName()
    {
        return m_IdmTypeName;
    }

    CutToAlleleLikelihood* CutToAlleleLikelihoodCollection::CreateObject()
    {
        return new CutToAlleleLikelihood( m_pGenes );
    }



    // ------------------------------------------------------------------------
    // --- MaternalDeposition
    // ------------------------------------------------------------------------

    MaternalDeposition::MaternalDeposition( const VectorGeneCollection* pGenes,
                                            VectorGeneDriverCollection* pGeneDrivers )
        : JsonConfigurable()
        , m_pGenes( pGenes )
        , m_pGeneDrivers(pGeneDrivers)
        , m_Cas9Allele()
        , m_AlleleIndexToCut()
        , m_LocusIndexToCut()
        , m_CutToLikelihoods(  pGenes, "Likelihood_Per_Cas9_gRNA_From" )
    {
    }

    MaternalDeposition::~MaternalDeposition()
    {
    }

    bool MaternalDeposition::Configure( const Configuration* config )
    {

        std::set<std::string> allowed_allele_names = m_pGenes->GetDefinedAlleleNames();

        m_Cas9Allele.constraint_param = &allowed_allele_names;
        m_Cas9Allele.constraints = "VectorSpeciesParameters.<species>.Genes";
        initConfigTypeMap( "Cas9_gRNA_From", &m_Cas9Allele, MD_Cas9_gRNA_From_DESC_TEXT);

        jsonConfigurable::ConstrainedString allele_to_cut;
        allele_to_cut.constraint_param = &allowed_allele_names;
        allele_to_cut.constraints = "VectorSpeciesParameters.<species>.Genes";
        initConfigTypeMap( "Allele_To_Cut", &allele_to_cut, MD_Allele_To_Cut_DESC_TEXT );

        m_LocusIndexToCut  = m_pGenes->GetLocusIndex( allele_to_cut );
        m_AlleleIndexToCut = m_pGenes->GetAlleleIndex( allele_to_cut );

        bool ret = JsonConfigurable::Configure( config );
        std::string allele_to_copy; // don't want this showing up in Cut_To_Allele, because this is the drive allele

        if( ret && !JsonConfigurable::_dryrun )
        {
            VectorGeneDriverType::Enum cas9_driver_type = ( *m_pGeneDrivers )[0]->GetDriverType(); // all drivers are same driver type

            jsonConfigurable::ConstrainedString cas9_allele;
            cas9_allele.constraint_param = &allowed_allele_names;
            cas9_allele.constraints = "VectorSpeciesParameters.<species>.Genes";

            bool valid_driver_allele = false;
            for( int i = 0; i < m_pGeneDrivers->Size(); i++ )
            {
                auto* driver = ( *m_pGeneDrivers )[i];
                std::string driver_name = m_pGenes->GetAlleleName( driver->GetDriverLocusIndex(), driver->GetDriverAlleleIndex() );

                // check that cas9 allele is a valid driver allele
                if( m_Cas9Allele == driver_name )
                {
                    // if DAISY_CHAIN, the driver does not have cas9_grna to cut the same locus, though it's defined
                    if( cas9_driver_type == VectorGeneDriverType::DAISY_CHAIN && m_LocusIndexToCut == driver->GetDriverLocusIndex() )
                    {
                        std::stringstream ss;
                        ss << "The 'Allele_To_Cut' ," << allele_to_cut << ", is not a valid allele for the 'Driving_Allele' allele " << m_Cas9Allele;
                        ss << " listed in 'Cas9_gRNA_From'.\n For 'DAISY_CHAIN' drive, the 'Driving_Allele' cannot cut the same locus (cannot drive itself).\n";
                        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                    }

                    // check that allele_to_cut is one of the allele_to_replace for this driver
                    std::stringstream ss;
                    ss << "The 'Allele_To_Cut' ," << allele_to_cut << ", is not a valid allele for the 'Driving_Allele' allele " << m_Cas9Allele;
                    ss << " listed in 'Cas9_gRNA_From'.\nThe 'Allele_To_Cut' must be one of the 'Allele_To_Replace' alleles defined in";
                    ss << " 'Alleles_Driven' where allele " << m_Cas9Allele << " is the 'Driving_Allele' .\n";
                    auto* driven_allele = driver->GetAlleleDriven( m_LocusIndexToCut );

                    if( driven_allele == nullptr ) {
                        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                    }
                    if( driven_allele->GetAlleleIndexToReplace() != m_AlleleIndexToCut ) {
                        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                    }
                    valid_driver_allele = true;
                    allele_to_copy = m_pGenes->GetAlleleName( driven_allele->GetLocusIndex(), driven_allele->GetAlleleIndexToCopy() );
                    break;
                }
            }

            if( !valid_driver_allele )
            {
                std::stringstream ss;
                ss << "The 'Cas9_gRNA_From' allele " << m_Cas9Allele << " is not one of the 'Driving_Allele' alleles defined in 'Drivers', but it should be.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );      
            }
         
        }

        if( ret )
        {
            // ---------------------------------------------------------------------------------------
            // --- The Cut_To_Allele and Likelihoods parameters must be configured after the Allele_To_Cut is configured.  
            // --- Cut_To_Allele needs this information when verifying that the allele read in are valid and
            // --- same locus as Allele_To_Cut.
            // ---------------------------------------------------------------------------------------

            initConfigComplexCollectionType( "Likelihood_Per_Cas9_gRNA_From", &m_CutToLikelihoods, MD_Likelihood_Per_Cas9_gRNA_From_DESC_TEXT);

            ret = JsonConfigurable::Configure( config );
            if( ret && !JsonConfigurable::_dryrun )
            {
                m_CutToLikelihoods.CheckConfiguration();
                std::string collection_name = m_CutToLikelihoods.GetCollectionName();
                bool found = false;
                float total_prob = 0.0;
                for( int i = 0; i < m_CutToLikelihoods.Size(); ++i )
                {
                    const CutToAlleleLikelihood* p_ctl = static_cast<CutToAlleleLikelihood*>(  m_CutToLikelihoods[i] );
                    std::string cut_to_allele_name = p_ctl->GetCopyToAlleleName();
                    uint8_t     cut_to_locus_index = m_pGenes->GetLocusIndex( cut_to_allele_name );
                    if( cut_to_locus_index != m_LocusIndexToCut )
                    {
                        std::stringstream ss;
                        ss << "Invalid allele defined in '" << collection_name << "'.\n";
                        ss << "The allele='" << cut_to_allele_name << "' is invalid with 'Allele_To_Cut'='" << allele_to_cut << "'.\n";
                        ss << "The allele defined in "<< collection_name << " must be of the same gene / locus as 'Allele_To_Cut'.";
                        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                    }
                    if( allele_to_copy == cut_to_allele_name )
                    {
                        std::stringstream ss;
                        ss << "Invalid allele in '"<< collection_name << "'.\n";
                        ss << "The 'Cut_To_Allele'='" << cut_to_allele_name << "' cannot be an 'Allele_To_Copy' in a drive.";
                        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                    }

                    if( cut_to_allele_name == allele_to_cut )
                    {
                        found = true;
                    }
                    total_prob += p_ctl->GetLikelihood();
                }
                if( fabs( 1.0 - total_prob ) > FLT_EPSILON )
                {
                    std::stringstream ss;
                    ss << "Invalid " << collection_name << " probabilities for 'Allele_To_Cut' = '" << allele_to_cut << "'.\n";
                    ss << "The sum of the probabilities equals " << total_prob << " but they must sum to 1.0.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                if( !found )
                {
                    std::stringstream ss;
                    ss << "Missing allele in " << collection_name << ".\n";
                    ss << "The 'Allele_To_Cut'='" << allele_to_cut << "' must have an entry in the " << collection_name << " list.\n";
                    ss << "The value represents probability of Maternal Deposition not affecting the 'Allele_To_Cut'.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                
            }
        }
        return ret;
    }

    const CutToAlleleLikelihoodCollection& MaternalDeposition::GetCutToLikelihoods() const
    {
        return m_CutToLikelihoods;
    }
    void MaternalDeposition::CheckRedifinition( const MaternalDeposition& rThat ) const
    {
        // Returns True when Allele_To_Cut is the same and there's an overlap in Cas9_gRNA_From alleles
        if( this->m_LocusIndexToCut  != rThat.m_LocusIndexToCut ) return;
        if( this->m_AlleleIndexToCut != rThat.m_AlleleIndexToCut ) return;
        if( this->m_Cas9Allele != rThat.m_Cas9Allele ) return;

        std::stringstream ss;
        ss << "At least two 'Maternal_Deposition' definitions with 'Allele_To_Cut'='";
        ss << m_pGenes->GetAlleleName( this->m_LocusIndexToCut, this->m_AlleleIndexToCut ) << "'\n";
        ss << " and 'Cas9_gRNA_From' allele ='" << this->m_Cas9Allele << "'.\n";
        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );

        return;
    }

    uint8_t MaternalDeposition::GetLocusIndexToCut() const
    {
        return m_LocusIndexToCut;
    }

    uint8_t MaternalDeposition::GetAlleleIndexToCut() const
    {
        return m_AlleleIndexToCut;
    }

    const std::string MaternalDeposition::GetCas9Allele() const
    {
        return m_Cas9Allele;
    }

    uint8_t MaternalDeposition::MomCas9AlleleCount( const VectorGenome & rMomGenome ) const
    {
        uint8_t total_ca9_alleles = 0;
        uint8_t cas9_locus = m_pGenes->GetLocusIndex( m_Cas9Allele );
        uint8_t cas9_index = m_pGenes->GetAlleleIndex( m_Cas9Allele );

        std::pair<uint8_t, uint8_t> mom_driver_indexes = rMomGenome.GetLocus( cas9_locus );
        if( mom_driver_indexes.first == cas9_index )
        {
            total_ca9_alleles = +1;
        }
        else if( mom_driver_indexes.second == cas9_index )
        {
            total_ca9_alleles = +1;
        }
        return total_ca9_alleles;
    }

    void MaternalDeposition::DoMaternalDeposition( uint8_t total_ca9_alleles_in_mom, GameteProbPairVector_t& rGametes ) const
    {
        release_assert( (total_ca9_alleles_in_mom == 1 || total_ca9_alleles_in_mom == 2) ); // only call this when at least one Cas9_gRNA_From allele present

        const CutToAlleleLikelihoodCollection& r_likelihoods = GetCutToLikelihoods();
        
        GameteProbPairVector_t new_gametes = rGametes;
        rGametes.clear();
        for( uint8_t i = 0; i < total_ca9_alleles_in_mom; ++i )
        {
            GameteProbPairVector_t tmp_gpp_list = new_gametes;
            new_gametes.clear();
            for( auto& r_gpp : tmp_gpp_list )
            {
                if( r_gpp.prob == 0.0 ) continue;
                uint8_t current_allele = r_gpp.gamete.GetLocus( m_LocusIndexToCut );
                if( current_allele != m_AlleleIndexToCut ) continue;

                for( int i = 0; i < r_likelihoods.Size(); ++i )
                {
                    const CutToAlleleLikelihood* p_ctl = static_cast<const CutToAlleleLikelihood*>( r_likelihoods[i] );
                    if( p_ctl->GetLikelihood() == 0.0 ) continue;
                    GameteProbPair gpp = r_gpp;
                    gpp.gamete.SetLocus( m_LocusIndexToCut, p_ctl->GetCopyToAlleleIndex() ); // wasting this on setting it back to same allele once per likelihoods?
                    gpp.prob *= p_ctl->GetLikelihood();
                    new_gametes.push_back( gpp );
                }
            }
        }


        // consolidate new gametes, group like, refill rGameters
        for( auto dm_gpp : new_gametes )
        {
            bool found = false;
            for( int i = 0; !found && ( i < rGametes.size() ); ++i )
            {
                if( rGametes[i].gamete == dm_gpp.gamete )
                {
                    rGametes[i].prob += dm_gpp.prob;
                    found = true;
                }
            }
            if( !found )
            {
                rGametes.push_back( dm_gpp );
            }
        }
    }



    // ------------------------------------------------------------------------
    // --- MaternalDepositionCollection
    // ------------------------------------------------------------------------

    VectorMaternalDepositionCollection::VectorMaternalDepositionCollection( const VectorGeneCollection* pGenes,
                                                                            VectorGeneDriverCollection* pGeneDrivers)
        : JsonConfigurableCollection( "vector MaternalDeposition" )
        , m_pGenes( pGenes )
        , m_pGeneDrivers( pGeneDrivers )
    {
    }

    VectorMaternalDepositionCollection::~VectorMaternalDepositionCollection()
    {
    }

    void VectorMaternalDepositionCollection::CheckConfiguration()
    {
        if( m_pGeneDrivers->Size() == 0 )
        {
            std::stringstream ss;
            ss << "'Maternal_Deposition' cannot happen without Gene Drive, please define at least one drivers in 'Drivers'.";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        for( int i = 0; i < m_Collection.size(); ++i )
        {
            MaternalDeposition* p_md_i = m_Collection[i];
            for( int j = i + 1; j < m_Collection.size(); ++j )
            {
                MaternalDeposition* p_md_j = m_Collection[j];
                p_md_i->CheckRedifinition( *p_md_j );
            }
        }
    }

    void VectorMaternalDepositionCollection::DoMaternalDeposition( const VectorGenome& rMomGenome, GameteProbPairVector_t& rGametes ) const
    {
        for( auto p_md : m_Collection )
        {
            uint8_t num_cas9_in_mom = p_md->MomCas9AlleleCount( rMomGenome );
            if( num_cas9_in_mom == 0 ) continue;

            p_md->DoMaternalDeposition( num_cas9_in_mom, rGametes);
        }
       
    }

    MaternalDeposition* VectorMaternalDepositionCollection::CreateObject()
    {
        return new MaternalDeposition( m_pGenes, m_pGeneDrivers);
    }

}