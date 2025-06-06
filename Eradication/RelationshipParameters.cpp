
#include "stdafx.h"
#include "RelationshipParameters.h"
#include "demographic_params.rc"

SETUP_LOGGING( "RelationshipParameters" )

namespace Kernel 
{
    RelationshipParameters::RelationshipParameters( RelationshipType::Enum type )
        : JsonConfigurable()
        , m_Type( type )
        , m_CoitalActRate(0.0)
        , m_DurationWeibullHeterogeneity(0.0)
        , m_DurationWeibullScale(0.0)
        , m_CondomUsage()
        , m_MigrationActions()
        , m_MigrationActionsCDF()
        , m_OverrideCoitalActRate( -1.0 )
        , m_pOverrideCondomUsage( nullptr )
        , m_OverrideDurationWeibullHeterogeneity( -1.0 )
        , m_OverrideDurationWeibullScale( -1.0 )
    {
    }

    RelationshipParameters::~RelationshipParameters()
    {
    }

    bool RelationshipParameters::Configure( const Configuration* config )
    {
        initConfigTypeMap( "Coital_Act_Rate",                &m_CoitalActRate,                Coital_Act_Rate_DESC_TEXT, FLT_EPSILON,    20.0f,  0.33f );
        initConfigTypeMap( "Duration_Weibull_Heterogeneity", &m_DurationWeibullHeterogeneity, Duration_Weibull_Heterogeneity_DESC_TEXT,   0,    100.0f, 1 );
        initConfigTypeMap( "Duration_Weibull_Scale",         &m_DurationWeibullScale,         Duration_Weibull_Scale_DESC_TEXT,           0,   FLT_MAX, 1 );
        initConfigTypeMap( "Condom_Usage_Probability",       &m_CondomUsage,                  Condom_Usage_DESC_TEXT );

        std::vector<std::string> ma_strings;
        std::vector<float> ma_dist;
        initConfigTypeMap( "Migration_Actions",              &ma_strings, Migration_Actions_DESC_TEXT );
        initConfigTypeMap( "Migration_Actions_Distribution", &ma_dist,    Migration_Actions_Distribution_DESC_TEXT, 0, 1 );

        bool ret = JsonConfigurable::Configure( config );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( ma_strings.size() != ma_dist.size() )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Migration_Actions.size()", int( ma_strings.size() ), 
                                                                                          "Migration_Actions_Distribution.size()", int( ma_dist.size() ),
                                                                                          "'Migration_Actions' and 'Migration_Actions_Distribution' must have the same number of elements.");
            }
            else if( ma_strings.size() == 0 )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "There must be at least one Migration Action defined." );
            }

            for( auto ma_str : ma_strings )
            {
                int rma = RelationshipMigrationAction::pairs::lookup_value( ma_str.c_str() );
                if( rma == -1 )
                {
                    std::stringstream ss;
                    ss << "'" << ma_str << "' is an unknown Migration_Action.  Possible actions are: ";
                    for( int i = 0 ; i < RelationshipMigrationAction::pairs::count() ; i++ )
                    {
                        ss << "'" << RelationshipMigrationAction::pairs::get_keys()[i] << "'";
                        if( (i+1) < RelationshipMigrationAction::pairs::count() )
                        {
                            ss << ", ";
                        }
                    }
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                if( std::find( m_MigrationActions.begin(), m_MigrationActions.end(), (RelationshipMigrationAction::Enum)rma ) != m_MigrationActions.end() )
                {
                    std::stringstream ss;
                    ss << "Duplicate Migration_Action found = '" << ma_str << "'.  There can be only one of each MigrationAction type.";
                    throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
                }
                m_MigrationActions.push_back( (RelationshipMigrationAction::Enum)rma );
            }

            float sum = 0.0;
            for( auto dist : ma_dist )
            {
                sum += dist ;
                m_MigrationActionsCDF.push_back( sum );
            }
            if( (sum < 0.9999) || (1.0001 < sum) )
            {
                std::ostringstream msg;
                msg << "The Migration_Actions_Distribution values do not add up to 1.0.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
            // ensure that the last value in the CDF is exactly 1
            m_MigrationActionsCDF[ m_MigrationActionsCDF.size()-1 ] = 1.0 ;
        }
        return ret;
    }

    RelationshipType::Enum RelationshipParameters::GetType() const 
    {
        return m_Type;
    }

    float RelationshipParameters::GetCoitalActRate() const
    {
        if( m_OverrideCoitalActRate >= 0.0 )
        {
            return m_OverrideCoitalActRate;
        }
        else
        {
            return m_CoitalActRate;
        }
    }

    float RelationshipParameters::GetDurationWeibullHeterogeneity() const
    {
        if( m_OverrideDurationWeibullHeterogeneity >= 0.0 )
        {
            return m_OverrideDurationWeibullHeterogeneity;
        }
        else
        {
            return m_DurationWeibullHeterogeneity;
        }
    }

    float RelationshipParameters::GetDurationWeibullScale() const
    {
        if( m_OverrideDurationWeibullScale >= 0.0 )
        {
            return m_OverrideDurationWeibullScale;
        }
        else
        {
            return m_DurationWeibullScale;
        }
    }

    const Sigmoid& RelationshipParameters::GetCondomUsage() const
    {
        if( m_pOverrideCondomUsage != nullptr )
        {
            return *m_pOverrideCondomUsage;
        }
        else
        {
            return m_CondomUsage;
        }
    }

    const std::vector<RelationshipMigrationAction::Enum>& RelationshipParameters::GetMigrationActions() const
    {
        return m_MigrationActions;
    }

    const std::vector<float>& RelationshipParameters::GetMigrationActionsCDF() const
    {
        return m_MigrationActionsCDF;
    }

    void RelationshipParameters::SetOverrideCoitalActRate( float overrideRate )
    {
        m_OverrideCoitalActRate = overrideRate;
    }

    void RelationshipParameters::SetOverrideCondomUsageProbability( const Sigmoid* pOverride )
    {
        m_pOverrideCondomUsage = pOverride;
    }

    void RelationshipParameters::SetOverrideRelationshipDuration( float heterogeniety, float scale )
    {
        m_OverrideDurationWeibullHeterogeneity = heterogeniety;
        m_OverrideDurationWeibullScale         = scale;
    }
}
