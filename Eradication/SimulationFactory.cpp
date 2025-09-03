
#include "stdafx.h"

#pragma warning(disable:4996)

#ifdef WIN32
#include "windows.h"
#endif
#include "Debug.h"
#include "Environment.h"
#include "Configuration.h"
#include "EventTrigger.h"

#include "Simulation.h"
#include "SimulationFactory.h"
#include "NodeProperties.h"
#include "Properties.h"

#ifndef _DLLS_
#ifndef DISABLE_MALARIA
#include "SimulationMalaria.h"
#endif
#ifndef DISABLE_VECTOR
#include "SimulationVector.h"
#endif

#ifndef DISABLE_STI
#include "SimulationSTI.h"
#endif
#ifndef DISABLE_HIV
#include "SimulationHIV.h"
#endif
#endif

#include "SerializedPopulation.h"

#include <chrono>
#include "EventTrigger.h"
#include "EventTriggerNode.h"
#include "EventTriggerCoordinator.h"
#include "RandomNumberGeneratorFactory.h"
#include "SerializationParameters.h"

SETUP_LOGGING( "SimulationFactory" )

namespace Kernel
{
    ISimulation * SimulationFactory::CreateSimulation()
    {
        EventTriggerFactory::GetInstance()->Configure( EnvPtr->Config );
        EventTriggerNodeFactory::GetInstance()->Configure( EnvPtr->Config );
        EventTriggerCoordinatorFactory::GetInstance()->Configure( EnvPtr->Config );
        NPFactory::CreateFactory();
        IPFactory::CreateFactory();

        ISimulation* newsim = nullptr;

        if ( SerializationParameters::GetInstance()->GetSerializedPopulationReadingType() != SerializationTypeRead::NONE )
        {
            const std::string population_filename = SerializationParameters::GetInstance()->GetSerializedPopulationFilename();
            LOG_INFO_F( "Loading serialized file: '%s'.\n", population_filename.c_str() );
            auto t_start = std::chrono::high_resolution_clock::now();
            newsim = SerializedState::LoadSerializedSimulation( population_filename.c_str() );
            auto t_finish = std::chrono::high_resolution_clock::now();
            newsim->Initialize( EnvPtr->Config );
            double elapsed = uint64_t((t_finish - t_start).count()) * 1000 * double(std::chrono::high_resolution_clock::period::num) / double(std::chrono::high_resolution_clock::period::den);
            LOG_DEBUG_F( "Loaded serialized population from '%s' in %f ms\n.", population_filename.c_str(), elapsed );
            return newsim;
        }

        std::string sSimType;

        try
        {
            sSimType = GET_CONFIG_STRING(EnvPtr->Config, "Simulation_Type");

            SimType::Enum sim_type;
            if (sSimType == "GENERIC_SIM")
                sim_type = SimType::GENERIC_SIM;
#ifndef DISABLE_VECTOR
            else if (sSimType == "VECTOR_SIM")
                sim_type = SimType::VECTOR_SIM;
#endif
#ifndef DISABLE_MALARIA
            else if (sSimType == "MALARIA_SIM")
                sim_type = SimType::MALARIA_SIM;
#endif
#ifndef DISABLE_STI
            else if (sSimType == "STI_SIM")
                sim_type = SimType::STI_SIM;
#endif
#ifndef DISABLE_HIV
            else if (sSimType == "HIV_SIM")
                sim_type = SimType::HIV_SIM;
#endif // HIV
            else
            {
                std::ostringstream msg;
                msg << "Simulation_Type " << sSimType << " not recognized." << std::endl;
                throw Kernel::GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }

            switch (sim_type)
            {
                case SimType::GENERIC_SIM:
                    newsim = Simulation::CreateSimulation(EnvPtr->Config);
                break;
#ifndef DISABLE_VECTOR
                case SimType::VECTOR_SIM:
                    newsim = SimulationVector::CreateSimulation(EnvPtr->Config);
                break;
#endif
#ifndef DISABLE_MALARIA
                case SimType::MALARIA_SIM:
                    newsim = SimulationMalaria::CreateSimulation(EnvPtr->Config);
                break;
#endif

#ifndef DISABLE_STI
                case SimType::STI_SIM:
                    newsim = SimulationSTI::CreateSimulation(EnvPtr->Config);
                break;
#endif
#ifndef DISABLE_HIV 
                case SimType::HIV_SIM:
                    newsim = SimulationHIV::CreateSimulation(EnvPtr->Config);
                break;
#endif // HIV
                default: 
                // Is it even possible to get here anymore?  Won't the SimulationConfig error out earlier parsing the parameter-string?
                //("SimulationFactory::CreateSimulation(): Error, Simulation_Type %d is not implemented.\n", sim_type);
                //throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "sim_type", sim_type, SimType::pairs::lookup_key( sim_typ )); // JB
                break;
            }
            release_assert(newsim);
        }
        catch ( GeneralConfigurationException& e ) {
            throw e;
        }

        return newsim;
    }
}
