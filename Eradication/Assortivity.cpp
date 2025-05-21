
#include "stdafx.h"
#include "Assortivity.h"
#include "demographic_params.rc"
#include "Exceptions.h"
#include "RANDOM.h"
#include "IndividualEventContext.h"
#include "Node.h"
#include "SimulationConfig.h"
#include "Simulation.h"

SETUP_LOGGING( "Assortivity" )

namespace Kernel 
{
    BEGIN_QUERY_INTERFACE_BODY(Assortivity)
    END_QUERY_INTERFACE_BODY(Assortivity)

    std::string Assortivity::ValuesToString( const std::vector<std::string>& rList )
    {
        std::stringstream ss ;
        for( auto val : rList )
        {
            ss << "'" << val << "' " ;
        }
        return ss.str() ;
    }

    Assortivity::Assortivity( RelationshipType::Enum relType, RANDOMBASE* prng )
        : IAssortivity()
        , m_RelType( relType )
        , m_pRNG( prng )
        , m_Group( AssortivityGroup::NO_GROUP )
        , m_PropertyKey()
        , m_Axes()
        , m_WeightingMatrix()
        , m_StartYear( MIN_YEAR )
        , m_StartUsing(false)
    {
        //release_assert( m_pRNG != nullptr );
    }

    Assortivity::~Assortivity()
    {
    }

    void Assortivity::SetParameters( RANDOMBASE* prng )
    {
        m_pRNG = prng;
        release_assert( m_pRNG != nullptr );
    }

    bool Assortivity::Configure(const Configuration *config)
    {
        bool ret = false ;
        bool prev_use_defaults = JsonConfigurable::_useDefaults ;
        bool resetTrackMissing = JsonConfigurable::_track_missing;
        JsonConfigurable::_track_missing = false;
        JsonConfigurable::_useDefaults = false ;
        try
        {

            initConfig( "Group", m_Group, config, MetadataDescriptor::Enum("m_Group", Group_DESC_TEXT, MDD_ENUM_ARGS(AssortivityGroup)) ); 

            IPKeyParameter ip_key;
            if( JsonConfigurable::_dryrun || (m_Group != AssortivityGroup::NO_GROUP) )
            {
                initConfigTypeMap( "Axes", &m_Axes, Axes_DESC_TEXT );

                initConfigTypeMap( "Weighting_Matrix_RowMale_ColumnFemale", &m_WeightingMatrix, Weighting_Matrix_RowMale_ColumnFemale_DESC_TEXT, 0.0f, 1.0f );

                if( JsonConfigurable::_dryrun || (m_Group == AssortivityGroup::INDIVIDUAL_PROPERTY) )
                {
                    std::string rel_type_str = RelationshipType::pairs::lookup_key( m_RelType ) ;
                    std::string param_name = rel_type_str + std::string(":Property_Name") ;
                    ip_key.SetParameterName( param_name );
                    initConfigTypeMap( "Property_Name", &ip_key, Property_Name_DESC_TEXT );
                }
            }

            initConfigTypeMap( "Start_Year", &m_StartYear, Assortivity_Start_Year_DESC_TEXT, MIN_YEAR, MAX_YEAR, MIN_YEAR, "Group", "STI_COINFECTION_STATUS,HIV_INFECTION_STATUS,HIV_TESTED_POSITIVE_STATUS,HIV_RECEIVED_RESULTS_STATUS");
            AddConfigurationParameters( m_Group, config );

            ret = JsonConfigurable::Configure( config );
            if( ret && !JsonConfigurable::_dryrun )
            {
                m_PropertyKey = ip_key;
                if( UsesStartYear() && m_StartYear < Simulation::base_year )
                {
                    LOG_WARN_F("Start_Year (%f) specified before Base_Year (%f), for relationship type %s\n",
                                m_StartYear, Simulation::base_year, RelationshipType::pairs::lookup_key(m_RelType));
                }
            }

            JsonConfigurable::_useDefaults = prev_use_defaults ;
            JsonConfigurable::_track_missing = resetTrackMissing;
        }
        catch( DetailedException& e )
        {
            JsonConfigurable::_useDefaults = prev_use_defaults ;
            JsonConfigurable::_track_missing = resetTrackMissing;

            std::stringstream ss ;
            ss << e.GetMsg() << "\n" << "Was reading values for " << RelationshipType::pairs::lookup_key( m_RelType ) << "." ;
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        catch( const json::Exception &e )
        {
            JsonConfigurable::_useDefaults = prev_use_defaults ;
            JsonConfigurable::_track_missing = resetTrackMissing;

            std::stringstream ss ;
            ss << e.what() << "\n" << "Was reading values for " << RelationshipType::pairs::lookup_key( m_RelType ) << "." ;
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        if( ret )
        {
            if( m_Group == AssortivityGroup::STI_INFECTION_STATUS )
            {
                if( GET_CONFIGURABLE( SimulationConfig )->sim_type != SimType::STI_SIM )
                {
                    const char* sim_type_str = SimType::pairs::lookup_key( GET_CONFIGURABLE( SimulationConfig )->sim_type );
                    std::stringstream ss ;
                    ss << RelationshipType::pairs::lookup_key( GetRelationshipType() ) << ":Group"; 
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                                                            ss.str().c_str(), AssortivityGroup::pairs::lookup_key( GetGroup() ), 
                                                            "Simulation_Type", sim_type_str,
                                                            "STI_INFECTION_STATUS is only valid with STI_SIM." );
                }
                CheckAxesForTrueFalse();
                SortMatrixFalseTrue();
            }
            else if( m_Group == AssortivityGroup::INDIVIDUAL_PROPERTY )
            {
                if( !m_PropertyKey.IsValid() )
                {
                    std::stringstream ss ;
                    ss << RelationshipType::pairs::lookup_key( m_RelType ) << ":Property_Name must be defined and cannot be empty string." ;
                    throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                CheckAxesForProperty();
            }
            else if( m_Group != AssortivityGroup::NO_GROUP )
            {
                CheckDerivedValues();
            }

            if( m_Group != AssortivityGroup::NO_GROUP )
            {
                CheckMatrix();
            }
        }
        return ret ;
    }

    void Assortivity::CheckAxesForTrueFalse()
    {
        for( int i = 0 ; i < m_Axes.size() ; i++ )
        {
            std::transform( m_Axes[i].begin(), m_Axes[i].end(), m_Axes[i].begin(), ::toupper );
        }
        if( (m_Axes.size() != 2) ||
            ((m_Axes[0] != "TRUE" ) && (m_Axes[1] != "TRUE" )) ||
            ((m_Axes[0] != "FALSE") && (m_Axes[1] != "FALSE")) )
        {
            std::stringstream ss ;
            ss << "The " << RelationshipType::pairs::lookup_key( m_RelType ) << ":Group (" 
               << AssortivityGroup::pairs::lookup_key( m_Group ) <<") requires that the Axes names(="
               << ValuesToString( m_Axes )
               <<") are 'TRUE' and 'FALSE'.  Order is up to the user." ;
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__,  ss.str().c_str() );
        }
    }

    void Assortivity::CheckAxesForProperty()
    {
        IPKeyValueContainer property_values ;
        try
        {
            property_values = IPFactory::GetInstance()->GetIP( m_PropertyKey.ToString() )->GetValues<IPKeyValueContainer>();
        }
        catch( DetailedException& )
        {
            std::stringstream ss ;
            ss <<  RelationshipType::pairs::lookup_key( m_RelType ) 
               << ":Property_Name(=" << m_PropertyKey.ToString() << ") is not defined in the demographics." ;
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        std::set<std::string> values_as_set = property_values.GetValuesToStringSet();
        bool invalid_axes = property_values.Size() != m_Axes.size();
        for( int i = 0 ; !invalid_axes && (i < m_Axes.size()) ; i++ )
        {
            invalid_axes = (values_as_set.count( m_Axes[i] ) != 1 );
        }
        if( invalid_axes )
        {
            std::stringstream ss ;
            ss << "The " << RelationshipType::pairs::lookup_key( m_RelType ) << ":Group (" 
               << AssortivityGroup::pairs::lookup_key( m_Group ) <<") requires that the Axes names"
               << "(=" << ValuesToString( m_Axes ) << ") "
               << "match the property values"
               << "(=" << property_values.GetValuesToString() << ") "
               << "defined in the demographics for Property=" << m_PropertyKey.ToString() << "." ;
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, ss.str().c_str() ) ;
        }
    }

    void Assortivity::CheckMatrix()
    {
        bool invalid_matrix = m_WeightingMatrix.size() != m_Axes.size() ;
        for( int i = 0 ; !invalid_matrix && (i < m_WeightingMatrix.size()) ; i++ )
        {
            invalid_matrix = m_WeightingMatrix[i].size() != m_Axes.size() ;
        }
        if( invalid_matrix )
        {
            std::stringstream ss ;
            ss <<  "The " << RelationshipType::pairs::lookup_key( m_RelType ) 
               << ":Weighting Matrix must be a square matrix whose dimensions are equal to the number of Axes." ;
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        // -------------------------------------------------------------------
        // --- Also check that a single row or single column is not all zeros.
        // -------------------------------------------------------------------
        for( int i = 0 ; i < m_WeightingMatrix.size() ; i++ )
        {
            bool row_ok = false ;
            bool col_ok = false ;
            for( int j = 0 ; j < m_WeightingMatrix.size() ; j++ )
            {
                row_ok |= m_WeightingMatrix[ i ][ j ] > 0.0 ;
                col_ok |= m_WeightingMatrix[ j ][ i ] > 0.0 ;
            }
            if( !row_ok || !col_ok )
            {
                std::stringstream ss ;
                ss <<  "The " << RelationshipType::pairs::lookup_key( m_RelType ) 
                   << ":Weighting Matrix cannot have a row or column where all the values are zero.  " ;
                if( !row_ok )
                   ss << "Row " << (i+1) << " is all zeros." ;
                else
                   ss << "Column " << (i+1) << " is all zeros." ;
                throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
    }

    void Assortivity::SortMatrixFalseTrue()
    {
        if (m_Axes[0] == "TRUE")
        {
            m_Axes[0] = "FALSE";
            m_Axes[1] = "TRUE";

            float tmp = m_WeightingMatrix[0][0];
            m_WeightingMatrix[0][0] = m_WeightingMatrix[1][1];
            m_WeightingMatrix[1][1] = tmp;

            tmp = m_WeightingMatrix[0][1];
            m_WeightingMatrix[0][1] = m_WeightingMatrix[1][0];
            m_WeightingMatrix[1][0] = tmp;
        }
    }

    bool Assortivity::UsesStartYear() const
    {
        return (m_Group == AssortivityGroup::STI_COINFECTION_STATUS)
            || (m_Group == AssortivityGroup::HIV_INFECTION_STATUS)
            || (m_Group == AssortivityGroup::HIV_TESTED_POSITIVE_STATUS)
            || (m_Group == AssortivityGroup::HIV_RECEIVED_RESULTS_STATUS);
    }

    int GetIndexSTI( const Assortivity* pAssortivity, const IIndividualHumanSTI* pIndividual )
    {
        return (pIndividual->IsInfected() ? 1 : 0) ; 
    }

    int GetIndexStiCoInfection( const Assortivity* pAssortivity, const IIndividualHumanSTI* pIndividual )
    {
        return (pIndividual->HasSTICoInfection() ? 1 : 0) ; 
    }

    std::string GetStringValueIndividualProperty( const Assortivity* pAssortivity, const IIndividualHumanSTI* pIndividual )
    {
        IPKey key = pAssortivity->GetPropertyKey();
        return pIndividual->GetPropertiesConst().Get( key ).GetValueAsString() ;
    }

    IIndividualHumanSTI* Assortivity::SelectPartner( IIndividualHumanSTI* pPartnerA,
                                                     const list<IIndividualHumanSTI*>& potentialPartnerList )
    {
        release_assert( pPartnerA != nullptr );

        if( potentialPartnerList.size() <= 0 )
        {
            return nullptr ;
        }

        AssortivityGroup::Enum group = GetGroupToUse() ;

        IIndividualHumanSTI* p_partner_B = nullptr ;
        switch( group )
        {
            case AssortivityGroup::NO_GROUP:
                p_partner_B = potentialPartnerList.front();
                break;

            case AssortivityGroup::STI_INFECTION_STATUS:
                p_partner_B = FindPartner( pPartnerA, potentialPartnerList, GetIndexSTI );
                break;

            case AssortivityGroup::INDIVIDUAL_PROPERTY:
                p_partner_B = FindPartnerIP( pPartnerA, potentialPartnerList, GetStringValueIndividualProperty );
                break;

            case AssortivityGroup::STI_COINFECTION_STATUS:
                p_partner_B = FindPartner( pPartnerA, potentialPartnerList, GetIndexStiCoInfection );
                break;

            default:
                p_partner_B = SelectPartnerForExtendedGroups( group,  pPartnerA, potentialPartnerList );
        }

        return p_partner_B ;
    }

    IIndividualHumanSTI* Assortivity::SelectPartnerForExtendedGroups( AssortivityGroup::Enum group,
                                                                      IIndividualHumanSTI* pPartnerA,
                                                                      const list<IIndividualHumanSTI*>& potentialPartnerList )
    {
        throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "group", group, AssortivityGroup::pairs::lookup_key( group ) );
    }

    void Assortivity::Update( const IdmDateTime& rCurrentTime, float dt )
    {
        float current_year = rCurrentTime.Year() ;
        m_StartUsing = m_StartYear < current_year ;
    }

    AssortivityGroup::Enum Assortivity::GetGroupToUse() const
    {
        AssortivityGroup::Enum group = GetGroup() ;
        if( !m_StartUsing )
        {
            group = AssortivityGroup::NO_GROUP ;
        }
        return group ;
    }

    IIndividualHumanSTI* Assortivity::SelectPartnerFromScore( float total_score,
                                                              const std::vector<float>& rScores,
                                                              const std::vector<IIndividualHumanSTI*>& rPartners )
    {
        // -------------------------------------------------------------------------
        // --- Select the partner based on their score/probability of being selected
        // -------------------------------------------------------------------------
        release_assert(m_pRNG != nullptr);
        float ran_score = m_pRNG->e() * total_score;
        std::vector<float>::const_iterator it = std::lower_bound( rScores.begin(),
                                                                  rScores.end(),
                                                                  ran_score );
        if( it != rScores.end() )
        {
            // -------------------------------------------------------------------
            // --- In order to continue to get the exact same results as before,
            // --- we have the following code.  The old code did a linear search
            // --- checking to see if ran_score was strictly greather than each
            // --- each individuals score.  However, this new code uses lower_bound()
            // --- to do a binary search and lower_bound() stops when it finds a
            // --- a value that is greater than or equal to ran_score.  Hence, the
            // --- following code keeps looking to find the first value that is
            // --- strictly greater than the ran_score.
            // -------------------------------------------------------------------
            while( (it != rScores.end()) && (*it == ran_score) )
            {
                ++it;
            }
            if( it != rScores.end() )
            {
                int index = it - rScores.begin();
                return rPartners[ index ];
            }
        }

        return nullptr;
    }

#define PS_MAX_LIST (50000)
    static std::vector<float> PS_vector_score;
    static std::vector<IIndividualHumanSTI*> PS_vector_partner;

    IIndividualHumanSTI* Assortivity::FindPartner( IIndividualHumanSTI* pPartnerA,
                                                   const list<IIndividualHumanSTI*>& potentialPartnerList,
                                                   tGetIndexFunc func)
    {
        if( PS_vector_score.capacity() < PS_MAX_LIST )
        {
            PS_vector_score.reserve( PS_MAX_LIST );
            PS_vector_partner.reserve( PS_MAX_LIST );
        }
        PS_vector_score.clear();
        PS_vector_partner.clear();

        // --------------------------------------------------------------
        // --- Get the index into the matrix for the male/partnerA based 
        // --- on his attribute and the axes that the attribute is in
        // --------------------------------------------------------------
        int a_index = func(this, pPartnerA);

        // -----------------------------------------------------------------------
        // --- Find the score for each female/partnerB given this particular male
        // -----------------------------------------------------------------------
        float total_score = 0.0f;
        for( auto p_partner_B : potentialPartnerList )
        {
            int b_index = func( this, p_partner_B );
            float score = m_WeightingMatrix[ a_index ][ b_index ];
            if( (score > 0.0) && (PS_vector_score.size() < PS_MAX_LIST) )
            {
                total_score += score;
                PS_vector_score.push_back( total_score );
                PS_vector_partner.push_back( p_partner_B );
                if( PS_vector_score.size() >= PS_MAX_LIST )
                {
                    break;
                }
            }
        }

        return SelectPartnerFromScore( total_score, PS_vector_score, PS_vector_partner );
    }

    //static std::vector<PartnerScore> partner_score_list;
    IIndividualHumanSTI* Assortivity::FindPartnerIP( IIndividualHumanSTI* pPartnerA,
                                                     const list<IIndividualHumanSTI*>& potentialPartnerList,
                                                     tGetStringValueFunc func)
    {
        if( PS_vector_score.capacity() < PS_MAX_LIST )
        {
            PS_vector_score.reserve( PS_MAX_LIST );
            PS_vector_partner.reserve( PS_MAX_LIST );
        }
        PS_vector_score.clear();
        PS_vector_partner.clear();

        // --------------------------------------------------------------
        // --- Get the index into the matrix for the male/partnerA based 
        // --- on his attribute and the axes that the attribute is in
        // --------------------------------------------------------------
        int a_index = pPartnerA->GetAssortivityIndex(m_RelType);
        if (a_index == -1)
        {
            a_index = GetIndex(func(this, pPartnerA));
            pPartnerA->SetAssortivityIndex(m_RelType, a_index);
        }

        // -----------------------------------------------------------------------
        // --- Find the score for each female/partnerB given this particular male
        // -----------------------------------------------------------------------
        float total_score = 0.0f;
        for (auto p_partner_B : potentialPartnerList)
        {
            int b_index = p_partner_B->GetAssortivityIndex(m_RelType);
            if (b_index == -1)
            {
                b_index = GetIndex(func(this, p_partner_B));
                p_partner_B->SetAssortivityIndex(m_RelType, b_index);
            }
            float score = m_WeightingMatrix[a_index][b_index];
            if( (score > 0.0) && (PS_vector_score.size() < PS_MAX_LIST) )
            {
                total_score += score;
                PS_vector_score.push_back( total_score );
                PS_vector_partner.push_back( p_partner_B );
                if( PS_vector_score.size() >= PS_MAX_LIST )
                {
                    break;
                }
            }
        }

        return SelectPartnerFromScore( total_score, PS_vector_score, PS_vector_partner );
    }

    int Assortivity::GetIndex( const std::string& rStringValue )
    {
        int index = std::find( m_Axes.begin(), m_Axes.end(), rStringValue ) - m_Axes.begin() ;
        if( (0 > index) || (index >= m_Axes.size()) )
        {
            std::stringstream ss ;
            ss << "The value (" << rStringValue << ") was not one of the Axes names ("
                << ValuesToString( m_Axes ) << ")." ;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        return index ;
    }

    REGISTER_SERIALIZABLE(Assortivity);

    void Assortivity::serialize(IArchive& ar, Assortivity* obj)
    {
        Assortivity& sort = *obj;

        std::string key_name;
        if( ar.IsWriter() )
        {
            key_name = sort.m_PropertyKey.ToString();
        }

        ar.labelElement("m_RelType"        ) & (uint32_t&)sort.m_RelType;
        ar.labelElement("m_Group"          ) & (uint32_t&)sort.m_Group;
        ar.labelElement("m_PropertyName"   ) & key_name;
        ar.labelElement("m_Axes"           ) & sort.m_Axes;
        ar.labelElement("m_WeightingMatrix") & sort.m_WeightingMatrix;
        ar.labelElement("m_StartYear"      ) & sort.m_StartYear;
        ar.labelElement("m_StartUsing"     ) & sort.m_StartUsing;

        if( ar.IsReader() )
        {
            sort.m_PropertyKey = IPKey( key_name );
        }

        //RANDOMBASE*                     m_pRNG ;
    }
}
