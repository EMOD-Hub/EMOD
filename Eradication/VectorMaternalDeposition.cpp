
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
        : CopyToAlleleLikelihood ( pGenes,
                                   CUT_TO_ALLELE,
                                   MD_Cut_To_Allele_DESC_TEXT,
                                   MD_Cut_Likelihood_DESC_TEXT )
    {
    }

    CutToAlleleLikelihood::CutToAlleleLikelihood( const VectorGeneCollection* pGenes,
                           const std::string& rAlleleName,
                           uint8_t alleleIndex,
                           float likelihood )
        : CopyToAlleleLikelihood( pGenes,
                                  rAlleleName,
                                  alleleIndex,
                                  likelihood,
                                  CUT_TO_ALLELE,
                                  MD_Cut_To_Allele_DESC_TEXT,
                                  MD_Cut_Likelihood_DESC_TEXT )
    {
    }

    CutToAlleleLikelihood::~CutToAlleleLikelihood()
    {
    }


    // ------------------------------------------------------------------------
    // --- CutToAlleleLikelihoodCollection
    // ------------------------------------------------------------------------

    CutToAlleleLikelihoodCollection::CutToAlleleLikelihoodCollection( const VectorGeneCollection* pGenes)
        : CopyToAlleleLikelihoodCollection( pGenes,
                                            "Likelihood_Per_Cas9_gRNA_From" )
    {
    }

    CutToAlleleLikelihoodCollection::~CutToAlleleLikelihoodCollection()
    {
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
        , m_Cas9AlleleLocus( DEFAULT_INDEX )
        , m_Cas9AlleleIndex( DEFAULT_INDEX )
        , m_AlleleToCutIndex( DEFAULT_INDEX )
        , m_AlleleToCutLocus( DEFAULT_INDEX )
        , m_CutToLikelihoods( pGenes )
    {
    }

    MaternalDeposition::~MaternalDeposition()
    {
    }

    bool MaternalDeposition::Configure( const Configuration* config )
    {
        const std::set<std::string>& allowed_allele_names = m_pGenes->GetDefinedAlleleNames();

        jsonConfigurable::ConstrainedString cas9_allele;
        cas9_allele.constraint_param = &allowed_allele_names;
        cas9_allele.constraints = m_pGenes->GENE_CONSTRAINTS;
        initConfigTypeMap( "Cas9_gRNA_From", &cas9_allele, MD_Cas9_gRNA_From_DESC_TEXT);

        jsonConfigurable::ConstrainedString allele_to_cut;
        allele_to_cut.constraint_param = &allowed_allele_names;
        allele_to_cut.constraints = m_pGenes->GENE_CONSTRAINTS;
        initConfigTypeMap( "Allele_To_Cut", &allele_to_cut, MD_Allele_To_Cut_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( config );
        std::string allele_to_copy; // don't want this showing up in Cut_To_Allele, because this is the drive allele

        if( ret && !JsonConfigurable::_dryrun )
        {
            if( m_pGeneDrivers->Size() == 0 )
            {
                std::stringstream ss;
                ss << "The 'Maternal_Deposition' cannot happen without gene drive and there are no 'Drivers' defined. Please define 'Drivers'.\n";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            VectorGeneDriverType::Enum cas9_driver_type = ( *m_pGeneDrivers )[0]->GetDriverType(); // all drivers are same driver type

            m_AlleleToCutLocus = m_pGenes->GetLocusIndex(  allele_to_cut );
            m_AlleleToCutIndex = m_pGenes->GetAlleleIndex( allele_to_cut );
            m_Cas9AlleleLocus  = m_pGenes->GetLocusIndex(  cas9_allele );
            m_Cas9AlleleIndex  = m_pGenes->GetAlleleIndex( cas9_allele );

            for( int i = 0; i < m_pGeneDrivers->Size(); i++ )
            {
                auto* driver = ( *m_pGeneDrivers )[i];
                std::string driver_name = m_pGenes->GetAlleleName( driver->GetDriverLocusIndex(), driver->GetDriverAlleleIndex() );
                if( cas9_allele != driver_name ) continue;

                // if DAISY_CHAIN, the driver does not have cas9_grna to cut the same locus, though it's defined
                if( cas9_driver_type == VectorGeneDriverType::DAISY_CHAIN && m_AlleleToCutLocus== driver->GetDriverLocusIndex() )
                {
                    std::stringstream ss;
                    ss << "The 'Allele_To_Cut'='" << allele_to_cut << "' is not a valid allele for the 'Cas9_gRNA_From'='" << cas9_allele << "'.\n";
                    ss << "For 'DAISY_CHAIN' drive, the 'Driving_Allele' cannot cut the same locus (drive itself).\n";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }

                // check that allele_to_cut is one of the allele_to_replace for this driver
                auto* driven_allele = driver->GetAlleleDriven( m_AlleleToCutLocus );
                if( driven_allele == nullptr || driven_allele->GetAlleleIndexToReplace() != m_AlleleToCutIndex ) 
                {
                    std::stringstream ss;
                    ss << "The 'Allele_To_Cut'='" << allele_to_cut << "' is not a valid allele for the 'Cas9_gRNA_From'='" << cas9_allele;
                    ss << "'.\nThe 'Allele_To_Cut' must be one of the 'Allele_To_Replace' for 'Driving_Allele'='" << cas9_allele << "'.\n";

                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }

                allele_to_copy = m_pGenes->GetAlleleName( driven_allele->GetLocusIndex(), driven_allele->GetAlleleIndexToCopy() );
                break;
            }

            if( allele_to_copy.empty() )
            {
                std::stringstream ss;
                ss << "The 'Cas9_gRNA_From'='" << cas9_allele << "' is not one of the 'Driving_Allele' alleles defined in 'Drivers', but it should be.";
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
                    const std::string& cut_to_allele_name = p_ctl->GetCopyToAlleleName();
                    uint8_t     cut_to_locus_index = m_pGenes->GetLocusIndex( cut_to_allele_name );
                    if( cut_to_locus_index != m_AlleleToCutLocus )
                    {
                        std::stringstream ss;
                        ss << "For 'Cas9_gRNA_From'='" << cas9_allele << "' and 'Allele_To_Cut'='" << allele_to_cut << "', ";
                        ss << "the 'Cut_To_Allele'='" << cut_to_allele_name << "' in '"<< collection_name<<"' is invalid.\n";
                        ss << "'Cut_To_Allele' and 'Allele_To_Cut' must be on the same gene / locus.";
                        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                    }
                    // allele_to_copy is an allele that's being driven, replacing the wildtype
                    // driven alleles cannot be generated by Maternal Deposition (otherwise this would be a drive)
                    if( allele_to_copy == cut_to_allele_name ) 
                    {
                        std::stringstream ss;
                        ss << "Invalid 'Cut_To_Allele'='" << cut_to_allele_name << "' in '"<< collection_name << "' for 'Cas9_gRNA_From'='" << cas9_allele << "'.\n";
                        ss << "A 'Cut_To_Allele' cannot be an 'Allele_To_Copy' for a 'Driving_Allele' ('" << cas9_allele << "').";
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
                    ss << "Invalid '" << collection_name << "' probabilities for 'Cas9_gRNA_From'='"<< cas9_allele <<"' and 'Allele_To_Cut'='" << allele_to_cut << "'.\n";
                    ss << "The sum of the probabilities is " << total_prob << " but they must sum to 1.0.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                if( !found )
                {
                    std::stringstream ss;
                    ss << "Missing allele in '" << collection_name << "'.\n";
                    ss << "The 'Allele_To_Cut'='" << allele_to_cut << "' must have an entry in the '" << collection_name << "' list.\n";
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
    void MaternalDeposition::CheckRedefinition( const MaternalDeposition& rThat ) const
    {
        // Can't have two Maternal_Deposition definitions with the same Cas9_gRNA_From and Allele_To_Cut
        if( this->m_AlleleToCutLocus != rThat.m_AlleleToCutLocus ) return;
        if( this->m_AlleleToCutIndex != rThat.m_AlleleToCutIndex ) return;
        if( this->m_Cas9AlleleLocus  != rThat.m_Cas9AlleleLocus ) return;
        if( this->m_Cas9AlleleIndex  != rThat.m_Cas9AlleleIndex ) return;

        std::stringstream ss;
        ss << "At least two 'Maternal_Deposition' definitions with 'Cas9_gRNA_From' = '";
        ss << m_pGenes->GetAlleleName( this->m_Cas9AlleleLocus, this->m_Cas9AlleleIndex ) ;
        ss << "' and 'Allele_To_Cut' = '" << m_pGenes->GetAlleleName( this->m_AlleleToCutLocus, this->m_AlleleToCutIndex ) << "'.\n";
        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );

        return;
    }

    uint8_t MaternalDeposition::GetAlleleToCutLocus() const
    {
        return m_AlleleToCutLocus;
    }

    uint8_t MaternalDeposition::GetAlleleToCutIndex() const
    {
        return m_AlleleToCutIndex;
    }

    uint8_t MaternalDeposition::GetCas9AlleleLocus() const
    {
        return m_Cas9AlleleLocus;
    }

    uint8_t MaternalDeposition::GetCas9AlleleIndex() const
    {
        return m_Cas9AlleleIndex;
    }

    uint16_t MaternalDeposition::MomCas9AlleleCount( const VectorGenome & rMomGenome ) const
    {
        uint16_t total_ca9_alleles = 0;

        std::pair<uint8_t, uint8_t> mom_driver_indexes = rMomGenome.GetLocus( m_Cas9AlleleLocus );
        if( mom_driver_indexes.first == m_Cas9AlleleIndex )
        {
            total_ca9_alleles += 1;
        }
        if( mom_driver_indexes.second == m_Cas9AlleleIndex )
        {
            total_ca9_alleles += 1;
        }
        return total_ca9_alleles;
    }

    void MaternalDeposition::DoMaternalDeposition( uint16_t total_ca9_alleles_in_mom, GameteProbPairVector_t& rGametes ) const
    {
        release_assert( (total_ca9_alleles_in_mom == 1 || total_ca9_alleles_in_mom == 2) ); // only call this when at least one Cas9_gRNA_From allele present
        const CutToAlleleLikelihoodCollection& r_likelihoods = GetCutToLikelihoods();

        // Separate non-target and target gametes
        GameteProbPairVector_t non_target_gametes;

        float will_not_cut_prob = 0.0f;
        // Apply the cut/likelihood logic for each Cas9 allele
        for( uint16_t i = 0; i < total_ca9_alleles_in_mom; ++i ) 
        {
            GameteProbPairVector_t next_gametes;
            for( const auto& gpp : rGametes )
            {
                uint8_t allele_index = gpp.gamete.GetLocus( m_AlleleToCutLocus );
                if( allele_index != m_AlleleToCutIndex )
                {
                    non_target_gametes.push_back( gpp );
                    continue; // skip non-target gametes
                }
                for( int k = 0; k < r_likelihoods.Size(); ++k ) 
                {
                    const CopyToAlleleLikelihood* p_ctl = r_likelihoods[k];
                    float likelihood = p_ctl->GetLikelihood();
                    if( likelihood == will_not_cut_prob ) continue;
                    GameteProbPair new_gpp = gpp;
                    new_gpp.gamete.SetLocus( m_AlleleToCutLocus, p_ctl->GetCopyToAlleleIndex() );
                    new_gpp.prob *= likelihood;
                    next_gametes.push_back( new_gpp );
                 }
            }
            rGametes = std::move( next_gametes );
        }

        // Merge non-target and processed target gametes
        GameteProbPairVector_t consolidated;
        auto consolidate = [&consolidated]( const GameteProbPair& gpp )
            {
                for( auto& existing : consolidated )
                {
                    if( existing.gamete == gpp.gamete )
                    {
                        existing.prob += gpp.prob;
                        return;
                    }
                }
                consolidated.push_back( gpp );
            };

        for( const auto& gpp : non_target_gametes ) consolidate( gpp );
        for( const auto& gpp : rGametes ) consolidate( gpp );
        rGametes = std::move( consolidated );
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
        for( int i = 0; i < m_Collection.size(); ++i )
        {
            MaternalDeposition* p_md_i = m_Collection[i];
            for( int j = i + 1; j < m_Collection.size(); ++j )
            {
                MaternalDeposition* p_md_j = m_Collection[j];
                p_md_i->CheckRedefinition( *p_md_j );
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