
#include "stdafx.h"
#include <iostream>
#include <queue>
#include <iomanip> //setw(), setfill()
#include <algorithm>
#include <deque>

#include "Controller.h"
#include "FileSystem.h"
#include "Debug.h"
#include "Log.h"
#include "suids.hpp"
#include "SimulationConfig.h"
#include "SimulationFactory.h"
#include "Simulation.h"
#include "IdmMpi.h"

#include "SerializedPopulation.h"
#include "SerializationParameters.h"
#include "StatusReporter.h"


using namespace Kernel;

SETUP_LOGGING( "Controller" )


void StepSimulation(ISimulation* sim, float dt);

// Basic simulation main loop with reporting
void RunSimulation( ISimulation* sim, int steps, float dt )
{
    LOG_DEBUG( "RunSimulation\n" );

    bool use_full_precision = (SerializationParameters::GetInstance()->GetPrecision() == SerializationPrecision::FULL);

    // Calculate the sorted set of time steps to serialize
    std::deque< int32_t > serialization_time_steps = SerializationParameters::GetInstance()->GetSerializedTimeSteps(steps);

    for (int t = 0; t < steps; t++)
    {
        if (!serialization_time_steps.empty() && t == serialization_time_steps.front())
        {
            SerializedState::SaveSerializedSimulation( dynamic_cast<Simulation*>(sim), t, true, use_full_precision );
            serialization_time_steps.pop_front();
        }

        StepSimulation(sim, dt);

        if (EnvPtr->MPI.Rank == 0)
        {
            EnvPtr->getStatusReporter()->ReportProgress(t+1, steps);
        }

        if(sim->TimeToStop())
        {
            break;
        }
    }

    // Serialize the final state if it was requested
    bool final_in_list = std::find(serialization_time_steps.begin(), serialization_time_steps.end(), steps) != serialization_time_steps.end();
    if (!serialization_time_steps.empty() && final_in_list)
    {
        SerializedState::SaveSerializedSimulation( dynamic_cast<Simulation*>(sim), steps, true, use_full_precision );
    }
}

void StepSimulation(ISimulation* sim, float dt)
{
    sim->Update(dt);

    EnvPtr->Log->Flush();
}

bool DefaultController::execute_internal()
{

    using namespace Kernel;
    list<string> serialization_test_state_filenames;

    LOG_DEBUG("DefaultController::execute_internal()...\n");

    JsonConfigurable::_useDefaults = false;
    JsonConfigurable::_track_missing = true;
    SerializationParameters::GetInstance()->Configure( EnvPtr->Config );  // Has to be configured before CreateSimulation()
    JsonConfigurable::CheckMissingParameters();

    std::unique_ptr<ISimulation> sim( SimulationFactory::CreateSimulation() ); 

    if (nullptr == sim.get())
    {
        throw InitializationException( __FILE__, __LINE__, __FUNCTION__, "sim.get() returned NULL after call to CreateSimulation.\n" );
    }

    if (EnvPtr->MPI.Rank==0) { ostringstream oss; oss << "Beginning Simulation...";  EnvPtr->getStatusReporter()->ReportStatus(oss.str()); }

    // populate it
    LOG_DEBUG("DefaultController::execute_internal() populate simulation...\n");
    // Confusing variable name (JC::useDefaults); we want to collect all defaults for reporting. It's up to us as the calling function
    JsonConfigurable::_track_missing = true;
    if(sim->Populate())
    {
        // Need to reset back to false; will be set as appropriate by campaign related code after this based on
        // "Use_Defaults" in campaign.json.
        JsonConfigurable::_useDefaults = false;
        JsonConfigurable::CheckMissingParameters();


        // now try to run it
        // divide the simulation into stages according to requesting number of serialization test cycles
        float dt = GET_CONFIGURABLE(SimulationConfig)->Sim_Tstep;
        int simulation_steps = int(GET_CONFIGURABLE(SimulationConfig)->Sim_Duration)/dt;

#ifndef _DLLS_
        int remaining_steps = simulation_steps;

        for (int k= 0; remaining_steps > 0; k++)
        {
            int cycle_steps = min(remaining_steps, max(1, simulation_steps));
            if (cycle_steps > 0)
                RunSimulation( sim.get(), cycle_steps, dt );

            remaining_steps -= cycle_steps;
        }
#else // _DLLS_
        LOG_INFO( "Execute_internal(): Calling RunSimulation.\n" );
        RunSimulation( sim, simulation_steps, dt );
#endif
        sim->WriteReportsData();

        if (EnvPtr->MPI.Rank==0)
        {
            LogTimeInfo lti;
            EnvPtr->Log->GetLogInfo(lti);

            ostringstream oss;
            oss << "Done - " << lti.hours << ":" << setw(2) << setfill('0') << lti.mins << ":" << setw(2) << setfill('0') << lti.secs;
            EnvPtr->getStatusReporter()->ReportStatus(oss.str());
        }

        // cleanup serialization test state files
        for (auto& filename : serialization_test_state_filenames)
        {
            if( FileSystem::FileExists( filename ) )
                FileSystem::RemoveFile( filename );
        }

        LOG_INFO_F( "Exiting %s\n", __FUNCTION__ );
        EnvPtr->Log->Flush();

        return true;
    }
    return false;
}

bool DefaultController::Execute()
{
    return execute_internal();
}
