
#include "stdafx.h"
#include "ImportPressure.h"

#include "Exceptions.h"
#include "NodeEventContext.h"  // for ISporozoiteChallengeConsumer methods
#include "SimulationConfig.h"
#include "RANDOM.h"

SETUP_LOGGING( "ImportPressure" );

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(ImportPressure)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(ImportPressure)

    IMPLEMENT_FACTORY_REGISTERED(ImportPressure)

    ImportPressure::ImportPressure() 
        : duration_counter(0)
        , num_imports(0)
    {
        LOG_DEBUG_F( "ctor\n" );
        initSimTypes( 1, "GENERIC_SIM" );
    }

    ImportPressure::~ImportPressure() 
    {
        LOG_DEBUG_F( "dtor: expired = %d\n", expired );
    }

    bool ImportPressure::Configure( const Configuration * inputJson )
    {
        LOG_DEBUG_F( "%s\n", __FUNCTION__ );
        // TODO: specification for rate, seasonality, and age-biting function
        initConfigTypeMap( "Durations",              &durations,              IP_Durations_DESC_TEXT,               0, INT_MAX );
        initConfigTypeMap( "Daily_Import_Pressures", &daily_import_pressures, IP_Daily_Import_Pressures_DESC_TEXT , 0, FLT_MAX );

        bool configured = Outbreak::Configure( inputJson );

        if( configured && !JsonConfigurable::_dryrun )
        {
            if( durations.size() != (daily_import_pressures.size()) )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                    "ImportPressure intervention requires Durations must be the same size as Daily_Import_Pressures" );
            }
            if( durations.size() <= 0 )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                    "Empty Durations parameter in ImportPressure intervention." );
            }
            ConstructDistributionCalendar();
        }
        return configured;
    }

    void ImportPressure::ExtraConfiguration()
    {
    }

    bool ImportPressure::Distribute(INodeEventContext *context, IEventCoordinator2* pEC)
    {
        return BaseNodeIntervention::Distribute( context, pEC );
    }

    void ImportPressure::ConstructDistributionCalendar()  
    {
        LOG_DEBUG_F( "%s\n", __FUNCTION__ );
        durations_and_pressures.push_back(std::make_pair((NaturalNumber) 1000000000, (NonNegativeFloat) 0.0));
        while (!durations.empty())
        {
            NaturalNumber duration = durations.back();
            NonNegativeFloat pressure = daily_import_pressures.back();
            durations_and_pressures.push_back(std::make_pair(duration, pressure));
            durations.pop_back();
            daily_import_pressures.pop_back();
        }      
        // Needs code review. Not sure how to properly destroy this node targeted intervention; and have seen behavior in which
        // it seems to "run off the back" of the durations and pressures vector, picking up a large import pressure and blowing up the population.
        // The line above is a hacked safety against that eventuality, but instead it should be properly destroyed at expiration.  
    }

    void ImportPressure::SetContextTo(INodeEventContext *context) 
    { 
        LOG_DEBUG_F( "%s\n", __FUNCTION__ );
        parent = context; 
    }

    // The durations are used in sequence as a series of relative deltas from the Start_Day
    // of the campaign event.
    // t(now).......+t0....+t1...............+t2.+t3....+t4
    void ImportPressure::Update( float dt )
    {
        // Throw away any entries with durations less than current value of duration_counter, and reset duration_counter
        while( durations_and_pressures.size() > 0 && 
               duration_counter >= durations_and_pressures.back().first )
        {
            LOG_DEBUG_F( "Discarding input entry with duration/pressure of %f/%f\n", (float) durations_and_pressures.back().first, (float) durations_and_pressures.back().second );
            durations_and_pressures.pop_back();
            duration_counter = 0;
            LOG_DEBUG_F( "Remaining entries: %i\n", (int) durations_and_pressures.size());
        }

        if (durations_and_pressures.empty()) {
            expired = true;
        }

        NonNegativeFloat daily_import_pressure = 0.0f; // 0 -> FLT_MAX, mean 'target'
        if (duration_counter < durations_and_pressures.back().first)
        {
            duration_counter += dt;
            daily_import_pressure = durations_and_pressures.back().second;   
            LOG_DEBUG_F("Duration counter = %f, Total duration = %f, import_pressure = %0.2f\n", (float) duration_counter, (float) durations_and_pressures.back().first, (float) daily_import_pressure);
        }
 
        // Convert the Poisson rate into a number of events
        num_imports = parent->GetRng()->Poisson(daily_import_pressure*dt);
        if (num_imports == 0)
        {
            LOG_DEBUG_F( "Poisson draw returned 0\n" );
            return;
        }
        else
        {       
            LOG_DEBUG_F("Duration counter = %f, Total duration = %f, import_cases = %d\n", (float) duration_counter, (float) durations_and_pressures.back().first, (int) num_imports);
            IStrainIdentity* strain_identity = GetNewStrainIdentity( parent );

            IOutbreakConsumer *ioc;
            if (s_OK == parent->QueryInterface(GET_IID(IOutbreakConsumer), (void**)&ioc))
            {
                ioc->AddImportCases(strain_identity, import_age, num_imports, 1.0 );
            }
            delete strain_identity;
        }
    }
}

 
