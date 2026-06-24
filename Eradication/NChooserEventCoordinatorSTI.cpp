
#include "stdafx.h"

#include "NChooserEventCoordinatorSTI.h"
#include "SimulationConfig.h"
#include "Simulation.h"
#include "IdmDateTime.h"

SETUP_LOGGING( "NChooserEventCoordinatorSTI" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- TargetedDistributionSTI
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED(TargetedDistributionSTI,TargetedDistribution)
    END_QUERY_INTERFACE_DERIVED(TargetedDistributionSTI,TargetedDistribution)

    TargetedDistributionSTI::TargetedDistributionSTI( NChooserObjectFactory* pObjectFactory )
    : TargetedDistribution( pObjectFactory )
    , m_StartYear(1900.0)
    , m_EndYear(2200.0)
    {
    }

    TargetedDistributionSTI::~TargetedDistributionSTI()
    {
    }

    bool TargetedDistributionSTI::operator<( const TargetedDistribution& rThat ) const
    {
        const TargetedDistributionSTI& r_sti_that = static_cast<const TargetedDistributionSTI&>(rThat);

        return (this->m_StartYear < r_sti_that.m_StartYear);
    }

    void TargetedDistributionSTI::AddTimeConfiguration()
    {
        initConfigTypeMap("Start_Year", &m_StartYear, NC_STI_TD_Start_Year_DESC_TEXT, 1900.0,  2200.0, 1900.0 );
        initConfigTypeMap("End_Year",   &m_EndYear,   NC_STI_TD_End_Year_DESC_TEXT,   1900.0,  2200.0, 2200.0 );
    }

    void TargetedDistributionSTI::CheckTimePeriod() const
    {
        if( m_StartYear >= m_EndYear )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                "Start_Year", m_StartYear,
                "End_Year", m_EndYear,
                "Start_Year must be < End_Year" );
        }
    
        if( m_StartYear < Simulation::base_year )
        {
            LOG_WARN_F("Start_Year (%f) specified before Base_Year (%f)\n", m_StartYear, Simulation::base_year);
        }
    }

    float TargetedDistributionSTI::GetStartInDays() const
    {
        return m_StartYear * DAYSPERYEAR;
    }

    float TargetedDistributionSTI::GetEndInDays() const
    {
        return m_EndYear * DAYSPERYEAR;
    }

    float TargetedDistributionSTI::GetCurrentInDays( const IdmDateTime& rDateTime ) const
    {
        return rDateTime.Year() * DAYSPERYEAR;
    }

    void TargetedDistributionSTI::CheckOverlaped( const TargetedDistribution& rPrev ) const
    {
        const TargetedDistributionSTI& r_sti_prev = static_cast<const TargetedDistributionSTI&>(rPrev);

        if( r_sti_prev.m_EndYear > this->m_StartYear )
        {
            std::stringstream ss;
            ss << "'Distributions' cannot have time periods that overlap.  ";
            ss << "(" << r_sti_prev.m_StartYear << ", " << r_sti_prev.m_EndYear << ") vs (" << this->m_StartYear << ", " << this->m_EndYear << ")";
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    bool TargetedDistributionSTI::IsPastStart( const IdmDateTime& rDateTime ) const
    {
        return m_StartYear <= rDateTime.Year();
    }

    bool TargetedDistributionSTI::IsPastEnd( const IdmDateTime& rDateTime ) const
    {
        return m_EndYear <= rDateTime.Year();
    }

    // ------------------------------------------------------------------------
    // --- NChooserObjectFactorySTI
    // ------------------------------------------------------------------------

    NChooserObjectFactorySTI::NChooserObjectFactorySTI()
    {
    }

    NChooserObjectFactorySTI::~NChooserObjectFactorySTI()
    {
    }

    TargetedDistribution* NChooserObjectFactorySTI::CreateTargetedDistribution()
    {
        return new TargetedDistributionSTI( this );
    }

    // ------------------------------------------------------------------------
    // --- NChooserEventCoordinatorSTI
    // ------------------------------------------------------------------------

    IMPLEMENT_FACTORY_REGISTERED(NChooserEventCoordinatorSTI)

    NChooserEventCoordinatorSTI::NChooserEventCoordinatorSTI()
    : NChooserEventCoordinator( new NChooserObjectFactorySTI() )
    {
    }

    NChooserEventCoordinatorSTI::NChooserEventCoordinatorSTI( NChooserObjectFactory* pObjectFactory )
    : NChooserEventCoordinator( pObjectFactory )
    {
        release_assert( m_pObjectFactory );
    }

    NChooserEventCoordinatorSTI::~NChooserEventCoordinatorSTI()
    {
    }

    bool NChooserEventCoordinatorSTI::Configure( const Configuration * inputJson )
    {
        if( !JsonConfigurable::_dryrun &&
            (GET_CONFIGURABLE( SimulationConfig )->sim_type != SimType::STI_SIM) &&
            (GET_CONFIGURABLE( SimulationConfig )->sim_type != SimType::HIV_SIM) )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "NChooserEventCoordinatorSTI can only be used in STI and HIV simulations." );
        }
        return NChooserEventCoordinator::Configure( inputJson );
    }
}

