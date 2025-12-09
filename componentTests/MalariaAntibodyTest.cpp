#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "componentTests.h"
#include "MalariaAntibody.h"
#include "SusceptibilityMalaria.h"
#include "RANDOM.h"
#include "RandomFake.h"

using namespace Kernel;


SUITE( MalariaAntibodyTest )
{
    struct MalariaAntibodyFixture
    {
        float inv_microliters_blood;

        MalariaAntibodyFixture()
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            JsonConfigurable::missing_parameters_set.clear();

            SusceptibilityMalariaConfig::falciparumMSPVars = DEFAULT_MSP_VARIANTS;
            SusceptibilityMalariaConfig::falciparumNonSpecTypes = DEFAULT_NONSPECIFIC_TYPES;
            SusceptibilityMalariaConfig::falciparumPfEMP1Vars = DEFAULT_PFEMP1_VARIANTS;
            SusceptibilityMalariaConfig::memory_level                          =    0.340f;
            SusceptibilityMalariaConfig::hyperimmune_decay_rate                =    0.000f;
            SusceptibilityMalariaConfig::MSP1_antibody_growthrate              =    0.045f;
            SusceptibilityMalariaConfig::antibody_stimulation_c50              =   30.000f;
            SusceptibilityMalariaConfig::antibody_capacity_growthrate          =    0.090f;
            SusceptibilityMalariaConfig::minimum_adapted_response              =    0.050f;
            SusceptibilityMalariaConfig::non_specific_growth                   =    0.500f;
            SusceptibilityMalariaConfig::antibody_csp_decay_days               =   90.000f;
            SusceptibilityMalariaConfig::antibody_days_to_long_term_decay      =  730.000f;
            SusceptibilityMalariaConfig::antibody_long_term_decay_days         = 7300.000f;

            inv_microliters_blood = float(1 / ( (0.225 * (7300/DAYSPERYEAR) + 0.5) * 1e6 )); // 5 liters

            // JPS: Since the hyperimmunity decay time is set based on memory level,
            // memory level has a max of 0.35 to keep 0.4-memory_level from getting too close to zero
            // This sets the decay rate towards memory level so that the decay from antibody levels of 1 to levels of 0.4 is consistent
            SusceptibilityMalariaConfig::hyperimmune_decay_rate = -log((0.4f - SusceptibilityMalariaConfig::memory_level) / (1.0f - SusceptibilityMalariaConfig::memory_level)) / 120.0f;

        }

        ~MalariaAntibodyFixture()
        {
            Environment::Finalize();
            JsonConfigurable::missing_parameters_set.clear();
        }
    };

    TEST_FIXTURE( MalariaAntibodyFixture, TestGrowthAndDecay )
    {
        PSEUDO_DES rng( 42 );

        MalariaAntibody msp = MalariaAntibody::CreateAntibody( MalariaAntibodyType::MSP1, 22, 0.0f );

        CHECK_EQUAL( MalariaAntibodyType::MSP1, msp.GetAntibodyType() );
        CHECK_EQUAL( 22, msp.GetAntibodyVariant() );
        CHECK_EQUAL( 0.0f, msp.GetAntibodyCapacity() );
        CHECK_EQUAL( 0.0f, msp.GetAntibodyConcentration() );

        CHECK_EQUAL( -1, msp.GetActiveIndex() );
        msp.SetActiveIndex( 7 );
        CHECK_EQUAL( 7, msp.GetActiveIndex() );

        float current_time = 1.0f;
        float dt = 0.125; // gets updated 8 times per day

        // -------------------------------------------------------------------
        // --- NoAsexualCycle
        // --- Days 1-7 (=Incubation_Period_Constant)
        // --- There are no antigens during this time.
        // --- SusceptibilityMalaria::m_antigenic_flag = false
        // -------------------------------------------------------------------
        
        // --------------------------------------------------------------------------
        // --- HepatocyteRelease -> AsexualCycle
        // --- Day 7 & 8 - InfectionMalaria calls SusceptibilityMalaria::AddAntibody() 
        // --- which sets SusceptibilityMalaria::m_antigenic_flag to true.
        // --- InfectionMalaria::Update() increases the antigen count.
        // --------------------------------------------------------------------------
        float irbc_timer = 2.0;
        CHECK_EQUAL( 0, msp.GetAntigenCount() );
        while( irbc_timer > 0 )
        {
            // InfectionMalaria::Update()
            msp.IncreaseAntigenCount( 1, current_time, dt );
            CHECK_EQUAL( 1, msp.GetAntigenCount() );

            // SusceptibilityMalaria::Update() -> updateImmunityMSP()
            msp.UpdateAntibodyCapacity( dt, inv_microliters_blood );
            msp.UpdateAntibodyConcentration( dt );

            // SusceptibilityMalaria::Update()
            msp.ResetCounters();

            current_time += dt;
            irbc_timer -= dt;
        }
        CHECK_CLOSE( 0.0f, msp.GetAntibodyCapacity(), 0.0000001 );
        CHECK_CLOSE( 0.0f, msp.GetAntibodyConcentration(), 0.0000001 );

        // --------------------------------------------------------------------------
        // --- AsexualCycle
        // --- Day 9
        // --- the irbc_timer expires and we increase the antigen count for the MSP by
        // --- the total number of IRBC as part of InfectionMalaria::processEndOfAsexualCycle()
        // --- We also give it a single antigen count.
        // --- We reset the irbc_time = 2.
        // --------------------------------------------------------------------------
        irbc_timer = 2.0;

        // InfectionMalaria::processEndOfAsexualCycle() - 1000=total_irbc
        msp.IncreaseAntigenCount( 1000000, current_time, dt );

        // InfectionMalaria::Update()
        msp.IncreaseAntigenCount( 1, current_time, dt );
        CHECK_EQUAL( 1000001, msp.GetAntigenCount() );

        // SusceptibilityMalaria::Update() -> updateImmunityMSP()
        msp.UpdateAntibodyCapacity( dt, inv_microliters_blood );
        msp.UpdateAntibodyConcentration( dt );

        // SusceptibilityMalaria::Update()
        msp.ResetCounters();

        current_time += dt;
        irbc_timer -= dt;

        CHECK_EQUAL(    0, msp.GetAntigenCount() );
        CHECK_CLOSE( 0.000298f, msp.GetAntibodyCapacity(),      0.0000001 );
        CHECK_CLOSE( 0.0f,      msp.GetAntibodyConcentration(), 0.0000001 );

        // --------------------------------------------------------------------------
        // --- AsexualCycle
        // --- Day 9.125 to 11
        // --------------------------------------------------------------------------

        while( irbc_timer > 0 )
        {
            // InfectionMalaria::Update()
            msp.IncreaseAntigenCount( 1, current_time, dt );
            CHECK_EQUAL( 1, msp.GetAntigenCount() );

            // SusceptibilityMalaria::Update() -> updateImmunityMSP()
            msp.UpdateAntibodyCapacity( dt, inv_microliters_blood  );
            msp.UpdateAntibodyConcentration( dt );

            // SusceptibilityMalaria::Update()
            msp.ResetCounters();

            current_time += dt;
            irbc_timer -= dt;
        }

        CHECK_EQUAL(    0, msp.GetAntigenCount() );
        CHECK_CLOSE( 0.000298f, msp.GetAntibodyCapacity(),      0.0000001 );
        CHECK_CLOSE( 0.0f,      msp.GetAntibodyConcentration(), 0.0000001 );

        // --------------------------------------------------------------------------
        // --- AsexualCycle
        // --- Day 11
        // --------------------------------------------------------------------------
        int64_t total_irbc = 1000;
        while( current_time < 40.0 )
        {
            irbc_timer = 2.0;
            if( current_time > 29.0 )
            { 
                total_irbc /= 10;
            }
            else
            {
                total_irbc *= 10;
            }

            // InfectionMalaria::processEndOfAsexualCycle() - 1000=total_irbc
            msp.IncreaseAntigenCount( total_irbc, current_time, dt );

            while( irbc_timer > 0 )
            {
                // InfectionMalaria::Update()
                msp.IncreaseAntigenCount( 1, current_time, dt );

                // SusceptibilityMalaria::Update() -> updateImmunityMSP()
                msp.UpdateAntibodyCapacity( dt, inv_microliters_blood );
                msp.UpdateAntibodyConcentration( dt );

                // SusceptibilityMalaria::Update()
                msp.ResetCounters();
                //printf( "%f,%f,%f\n", current_time, msp.GetAntibodyCapacity(), msp.GetAntibodyConcentration() );

                current_time += dt;
                irbc_timer -= dt;
            }
        }
        CHECK_EQUAL(    0, msp.GetAntigenCount() );
        CHECK_CLOSE( 0.928295f, msp.GetAntibodyCapacity(),      0.000001 );
        CHECK_CLOSE( 0.925071f, msp.GetAntibodyConcentration(), 0.000001 );

        CHECK_EQUAL( 41, current_time );
        dt = 1.0;
        float last_capacity = msp.GetAntibodyCapacity();
        while( current_time < 14600.0 )
        {
            MalariaAntibody msp_copy( msp );
            msp_copy.IncreaseAntigenCount( 1, current_time, 0.125 );
            last_capacity = msp_copy.GetAntibodyCapacity();
            //printf( "%f,%f,%f\n", current_time, msp_copy.GetAntibodyCapacity(), msp_copy.GetAntibodyConcentration() );
            current_time += dt;
            if( current_time > 7300 )
            {
                dt = 100.0;
            }
            else if( current_time > 340 )
            {
                dt = 10.0;
            }
        }
        CHECK( last_capacity < 0.052 );
    }
}