
#include "stdafx.h"
#include <iostream>
#include <fstream>
#include "componentTests.h"
#include "UnitTest++.h"
#include "FakeLogger.h"

#include "SerializationParameters.h"
#include "Exceptions.h"
#include "SimulationConfig.h"

using namespace std; 
using namespace Kernel; 

SUITE(SerializationParametersTest)
{
    struct SerializationParametersFixture
    {
        std::unique_ptr<SimulationConfig> m_pSimulationConfig;
        FakeLogger m_fakeLogger;

        SerializationParametersFixture()
            : m_pSimulationConfig(new SimulationConfig()),
              m_fakeLogger(Logger::tLevel::WARNING)
        {
            Environment::Finalize();
            Environment::setSimulationConfig(m_pSimulationConfig.get());
            Environment::setLogger(&m_fakeLogger);

            m_pSimulationConfig->Sim_Tstep = 1;
            m_pSimulationConfig->starttime = 0;

            JsonConfigurable::_useDefaults = false;
        }

        ~SerializationParametersFixture()
        {
            Environment::Finalize();
            SerializationParameters::ResetInstance();
        }

        void TestHelper_ConfigureException( int lineNumber, const std::string& rFilename, const std::string& rExp, const std::string& rExpMsg )
        {
            try
            {
                unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename ) );
                SerializationParameters::GetInstance()->Configure( p_config.get() );
                CHECK_LN( false, lineNumber );
            }
            catch( DetailedException& re )
            {
                std::string msg = re.GetMsg();
                bool passed = ( msg.find( rExp ) != string::npos ) && ( msg.find( rExpMsg ) != string::npos );
                if( !passed )
                {
                    PrintDebug( rExpMsg );
                    PrintDebug( msg );
                    CHECK_LN( false, lineNumber );
                }
            }
        }
    };

    TEST_FIXTURE(SerializationParametersFixture, TestNothingProvided)
    {
        unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile("testdata/SerializationParametersTest/TestNothingProvided.json"));

        int steps = 10;

        SerializationParameters::GetInstance()->Configure(p_config.get());
        std::deque<int32_t> serialization_time_steps = SerializationParameters::GetInstance()->GetSerializedTimeSteps(steps);

        CHECK(serialization_time_steps.empty());
        CHECK(m_fakeLogger.Empty());
    }
    
    TEST_FIXTURE(SerializationParametersFixture, TestSimpleTimeSteps)
    {
        unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile("testdata/SerializationParametersTest/TestSimpleTimeSteps.json"));
        int steps = 10;
        
        SerializationParameters::GetInstance()->Configure(p_config.get());
        std::deque<int32_t> serialization_time_steps = SerializationParameters::GetInstance()->GetSerializedTimeSteps(steps);

        std::deque<int32_t> compared(
        { 0, 1, 2 }
        );
        CHECK_ARRAY_EQUAL(serialization_time_steps, compared, compared.size());
        CHECK(m_fakeLogger.Empty());

    }

    TEST_FIXTURE(SerializationParametersFixture, TestSimpleTimes)
    {
        unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile("testdata/SerializationParametersTest/TestSimpleTimes.json"));
        m_pSimulationConfig->Sim_Tstep = 1.5;
        m_pSimulationConfig->starttime = 1;
        int steps = 10;
        
        SerializationParameters::GetInstance()->Configure(p_config.get());
        std::deque<int32_t> serialization_time_steps = SerializationParameters::GetInstance()->GetSerializedTimeSteps(steps);

        std::deque<int32_t> compared(
        { 0, 1, 2 }
        );
        CHECK_ARRAY_EQUAL(serialization_time_steps, compared, compared.size());
        CHECK(m_fakeLogger.Empty());
    }

    TEST_FIXTURE(SerializationParametersFixture, TestNegativeOneStep)
    {
        unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile("testdata/SerializationParametersTest/TestNegativeOneStep.json"));
        int steps = 10;

        SerializationParameters::GetInstance()->Configure(p_config.get());
        std::deque<int32_t> serialization_time_steps = SerializationParameters::GetInstance()->GetSerializedTimeSteps(steps);

        std::deque<int32_t> compared(
        { 0, 1, 2, 10 }
        );
        CHECK_ARRAY_EQUAL(serialization_time_steps, compared, compared.size());
        CHECK(m_fakeLogger.Empty());
    }

    TEST_FIXTURE(SerializationParametersFixture, TestNegativeOneTime)
    {
        unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile("testdata/SerializationParametersTest/TestNegativeOneTime.json"));
        m_pSimulationConfig->Sim_Tstep = 1.5;
        m_pSimulationConfig->starttime = 1;
        int steps = 10;

        SerializationParameters::GetInstance()->Configure(p_config.get());
        std::deque<int32_t> serialization_time_steps = SerializationParameters::GetInstance()->GetSerializedTimeSteps(steps);

        std::deque<int32_t> compared(
        { 0, 1, 2, 10 }
        );
        CHECK_ARRAY_EQUAL(serialization_time_steps, compared, compared.size());
        CHECK(m_fakeLogger.Empty());
    }

    TEST_FIXTURE(SerializationParametersFixture, TestZeroTime)
    {
        unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile("testdata/SerializationParametersTest/TestZeroTime.json"));
        m_pSimulationConfig->Sim_Tstep = 1.5;
        m_pSimulationConfig->starttime = 10;
        int steps = 10;

        SerializationParameters::GetInstance()->Configure(p_config.get());
        std::deque<int32_t> serialization_time_steps = SerializationParameters::GetInstance()->GetSerializedTimeSteps(steps);

        std::deque<int32_t> compared(
        { 0, 1, 2, 10 }
        );
        CHECK_ARRAY_EQUAL(serialization_time_steps, compared, compared.size());
        CHECK(m_fakeLogger.Empty());
    }

    TEST_FIXTURE(SerializationParametersFixture, TestTimesBetweenSteps)
    {
        unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile("testdata/SerializationParametersTest/TestTimesBetweenSteps.json"));
        m_pSimulationConfig->Sim_Tstep = 1.5;
        m_pSimulationConfig->starttime = 1;
        int steps = 10;

        SerializationParameters::GetInstance()->Configure(p_config.get());
        std::deque<int32_t> serialization_time_steps = SerializationParameters::GetInstance()->GetSerializedTimeSteps(steps);

        std::deque<int32_t> compared(
        { 0, 1, 2 }
        );
        CHECK_ARRAY_EQUAL(serialization_time_steps, compared, compared.size());
        CHECK(m_fakeLogger.Empty());
    }

    TEST_FIXTURE(SerializationParametersFixture, TestDuplicateTimes)
    {
        unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile("testdata/SerializationParametersTest/TestDuplicateTimes.json"));
        m_pSimulationConfig->Sim_Tstep = 1.5;
        m_pSimulationConfig->starttime = 1;

        int steps = 10;

        SerializationParameters::GetInstance()->Configure(p_config.get());
        std::deque<int32_t> serialization_time_steps = SerializationParameters::GetInstance()->GetSerializedTimeSteps(steps);

        std::deque<int32_t> compared(
        { 0, 1, 2 }
        );
        CHECK_ARRAY_EQUAL(serialization_time_steps, compared, compared.size());
        LogEntry entry = m_fakeLogger.Back();
        CHECK_EQUAL(entry.log_level, Logger::tLevel::WARNING);
        CHECK(entry.msg.find("Duplicate serialization step specified (step=2, time=4.000000), removed from list\n") != string::npos);
        m_fakeLogger.Pop_Back();
        entry = m_fakeLogger.Back();
        CHECK_EQUAL(entry.log_level, Logger::tLevel::WARNING);
        CHECK(entry.msg.find("Duplicate serialization step specified (step=0, time=1.000000), removed from list\n") != string::npos);
        m_fakeLogger.Pop_Back();
        CHECK(m_fakeLogger.Empty());
    }

    TEST_FIXTURE(SerializationParametersFixture, TestDuplicateTimesteps)
    {
        unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile("testdata/SerializationParametersTest/TestDuplicateTimesteps.json"));

        int steps = 10;

        SerializationParameters::GetInstance()->Configure(p_config.get());
        std::deque<int32_t> serialization_time_steps = SerializationParameters::GetInstance()->GetSerializedTimeSteps(steps);

        std::deque<int32_t> compared(
        { 0, 1, 2 }
        );
        CHECK_ARRAY_EQUAL(serialization_time_steps, compared, compared.size());
        LogEntry entry = m_fakeLogger.Back();
        CHECK_EQUAL(entry.log_level, Logger::tLevel::WARNING);
        CHECK(entry.msg.find("Duplicate serialization step specified (step=2, time=2.000000), removed from list\n") != string::npos);
        m_fakeLogger.Pop_Back();
        entry = m_fakeLogger.Back();
        CHECK_EQUAL(entry.log_level, Logger::tLevel::WARNING);
        CHECK(entry.msg.find("Duplicate serialization step specified (step=0, time=0.000000), removed from list\n") != string::npos);
        m_fakeLogger.Pop_Back();
        CHECK(m_fakeLogger.Empty());
    }

    TEST_FIXTURE(SerializationParametersFixture, TestTooEarlyTime)
    {
        unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile("testdata/SerializationParametersTest/TestTooEarlyTime.json"));
        m_pSimulationConfig->Sim_Tstep = 1.5;
        m_pSimulationConfig->starttime = 1;

        int steps = 10;

        try
        {
            SerializationParameters::GetInstance()->Configure(p_config.get());
            std::deque<int32_t> serialization_time_steps = SerializationParameters::GetInstance()->GetSerializedTimeSteps(steps);
            CHECK(false); // Should not get here
        }
        catch (ConfigurationRangeException ex)
        {
            // Breaking the check across two messages to get around path being included in the exception
            std::string exMsg(ex.GetMsg());
            CHECK(exMsg.find("ConfigurationRangeException:") != string::npos); 
            CHECK(exMsg.find("Configuration variable 'Serialization_Times' with value -3 out of range: less than -1.") != string::npos);
            CHECK(m_fakeLogger.Empty());
        }
    }

    TEST_FIXTURE(SerializationParametersFixture, TestTooLateTime)
    {
        unique_ptr<Configuration> p_config(Environment::LoadConfigurationFile("testdata/SerializationParametersTest/TestTooLateTime.json"));
        m_pSimulationConfig->Sim_Tstep = 1.5;
        m_pSimulationConfig->starttime = 1;

        int steps = 10;

        SerializationParameters::GetInstance()->Configure(p_config.get());
        std::deque<int32_t> serialization_time_steps = SerializationParameters::GetInstance()->GetSerializedTimeSteps(steps);

        std::deque<int32_t> compared(
        { 0, 1, 2 }
        );
        CHECK_ARRAY_EQUAL(serialization_time_steps, compared, compared.size());
        LogEntry entry = m_fakeLogger.Back();
        CHECK_EQUAL(entry.log_level, Logger::tLevel::WARNING);
        CHECK(entry.msg.find("Out of range serialization step specified (step=11, time=17.500000), removed from list\n") != string::npos);
        m_fakeLogger.Pop_Back();
        CHECK(m_fakeLogger.Empty());
    }

    TEST_FIXTURE( SerializationParametersFixture, ReadWriteMask16 )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/SerializationParametersTest/TestReadWriteMask16.json" ) );

        SerializationParameters::GetInstance()->Configure( p_config.get() );
        SerializationBitMask_t read_mask = SerializationParameters::GetInstance()->GetSerializationReadMask();
        SerializationBitMask_t write_mask = SerializationParameters::GetInstance()->GetSerializationWriteMask();
        SerializationBitMask_t expected;
        expected.set( SerializationFlags::LarvalHabitats );
        CHECK_EQUAL( read_mask, expected );
        CHECK_EQUAL( write_mask, expected );
    }
 
    TEST_FIXTURE( SerializationParametersFixture, TestErrorMsgNoTimeInSerPop )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/SerializationParametersTest/TestErrorMsgNoTimeInSerPop.json", 
            "InvalidInputDataException", "No time in 'Serialization_Times'." );
    }

    TEST_FIXTURE( SerializationParametersFixture, TestErrorMsgNoTimestepInSerPop )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/SerializationParametersTest/TestErrorMsgNoTimestepInSerPop.json",
            "InvalidInputDataException", "No timestep in 'Serialization_Time_Steps'." );
    }

    TEST_FIXTURE( SerializationParametersFixture, TestErrorUnsupportedWriteMask )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/SerializationParametersTest/TestErrorUnsupportedWriteMask.json",
            "InvalidInputDataException", "Parameter 'Serialization_Mask_Node_Write' with value '17' is not supported. Currently only '16' and '0' are supported." );
    }

    TEST_FIXTURE( SerializationParametersFixture, TestErrorMsgMPINumTasks )
    {
        EnvPtr->MPI.NumTasks = 2;
        TestHelper_ConfigureException( __LINE__, "testdata/SerializationParametersTest/TestErrorMsgMPINumTasks.json",
            "IncoherentConfigurationException", "Number of files listed in 'Serialized_Population_Filenames' doesn't match number of MPI tasks." );
    }

    TEST_FIXTURE( SerializationParametersFixture, TestErrorMsgNoSerPopFiles )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/SerializationParametersTest/TestErrorMsgNoSerPopFiles.json",
            "InvalidInputDataException", "defined in 'Serialized_Population_Filenames' does not exist in 'Serialized_Population_Path' = '.'." );
    }

    TEST_FIXTURE( SerializationParametersFixture, TestErrorUnsupportedReadMask )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/SerializationParametersTest/TestErrorUnsupportedReadMask.json",
            "InvalidInputDataException", "Parameter 'Serialization_Mask_Node_Read' with value '17' is not supported. Currently only '16' and '0' are supported.");
    }
}

