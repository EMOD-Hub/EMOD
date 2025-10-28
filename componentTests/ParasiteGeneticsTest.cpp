
#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "componentTests.h"
#include "ParasiteGenetics.h"
#include "ParasiteGenome.h"
#include "SusceptibilityMalaria.h"
#include "RANDOM.h"
#include "RandomFake.h"
#include "JsonFullReader.h"
#include "JsonFullWriter.h"
#include "Memory.h"

using namespace Kernel;


SUITE( ParasiteGeneticsTest )
{
    struct ParasiteFixture
    {
        ParasiteFixture()
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
            JsonConfigurable::missing_parameters_set.clear();
            ParasiteGenome::ClearStatics();

            SusceptibilityMalariaConfig::falciparumMSPVars      = DEFAULT_MSP_VARIANTS;
            SusceptibilityMalariaConfig::falciparumNonSpecTypes = DEFAULT_NONSPECIFIC_TYPES;
            SusceptibilityMalariaConfig::falciparumPfEMP1Vars   = DEFAULT_PFEMP1_VARIANTS;

            // -----------------------------------------------------------------------------
            // --- There are 4 static ParasiteGenomeInner variables in ParasiteGenome.cpp.
            // --- We want to make sure that we are not leaking memory so we make sure that
            // --- we always start and end with our static 4.
            // -----------------------------------------------------------------------------
            CHECK_EQUAL( 4, ParasiteGenome::GetNumActive() );
        }

        ~ParasiteFixture()
        {
            // ------------------------------------------------------------------------
            // --- By using the normal routine to remove instances that only appear in
            // --- the map, we should be left with the map empty and only the 4 static
            // --- instances of ParasiteGenomeInner remaining.
            // ------------------------------------------------------------------------
            ParasiteGenetics::CreateInstance()->ReduceGenomeMap();
            CHECK_EQUAL( 4, ParasiteGenome::GetNumActive() );

            Environment::Finalize();
            ParasiteGenetics::DeleteInstance();
            ParasiteGenome::ClearStatics();
            JsonConfigurable::missing_parameters_set.clear();
        }

        void TestHelper_ConfigureException( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
        {
            try
            {
                EnvPtr->Config = Environment::LoadConfigurationFile( rFilename );

                ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

                CHECK_LN( false, lineNumber ); // should not get here
            }
            catch( DetailedException& re )
            {
                std::string msg = re.GetMsg();
                if( msg.find( rExpMsg ) == string::npos )
                {
                    PrintDebug( "\n" );
                    PrintDebug( rExpMsg );
                    PrintDebug( "\n" );
                    PrintDebug( msg );
                    CHECK_LN( false, lineNumber );
                }
            }
        }

    };

#if 0
    TEST_FIXTURE( ParasiteFixture, TestNumOocysts )
    {
        PSEUDO_DES rng( 42 );

        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ParasiteGeneticsTest/TestSuccessfulReadOfParameters.json" );

        ParasiteGenetics* p_pg = ParasiteGenetics::CreateInstance();
        p_pg->Configure( EnvPtr->Config );

        uint32_t num_bins = 11;
        uint32_t num_samples = 1000000;
        std::vector<uint32_t> histogram(num_bins,0);
        for( uint32_t i = 0; i < num_samples; ++i )
        {
            uint32_t num = p_pg->GetNumOocystsFromBite( &rng );
            if( num < num_bins-1 )
            {
                histogram[ num ] += 1;
            }
            else
            {
                histogram[ num_bins-1 ] += 1;
            }
        }
        for( uint32_t i = 0; i < num_bins; ++i )
        {
            printf( "%f\n", float(histogram[ i ])/float(num_samples) );
        }
        printf( "\n" );
    }

    TEST_FIXTURE( ParasiteFixture, TestNumSporozoites )
    {
        PSEUDO_DES rng( 42 );

        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ParasiteGeneticsTest/TestSuccessfulReadOfParameters.json" );

        ParasiteGenetics* p_pg = ParasiteGenetics::CreateInstance();
        p_pg->Configure( EnvPtr->Config );

        uint32_t num_bins = 30;
        uint32_t num_samples = 1000000;
        std::vector<uint32_t> histogram(num_bins,0);
        for( uint32_t i = 0; i < num_samples; ++i )
        {
            uint32_t num = p_pg->GetNumSporozoitesInBite( &rng );
            if( num < num_bins-1 )
            {
                histogram[ num ] += 1;
            }
            else
            {
                histogram[ num_bins-1 ] += 1;
            }
        }
        for( uint32_t i = 0; i < num_bins; ++i )
        {
            printf( "%f\n", float(histogram[ i ])/float(num_samples) );
        }
        printf( "\n" );
    }

    TEST_FIXTURE( ParasiteFixture, TestData )
    {
        PSEUDO_DES rng( 42 );

        SusceptibilityMalariaConfig::falciparumMSPVars = 100;
        SusceptibilityMalariaConfig::falciparumPfEMP1Vars = 1000;

        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ParasiteGeneticsTest/TestCreateGenomeFromBarcode.json" );

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        // ------------------------------------------------
        // --- Test the creation of a genome from a barcode
        // ------------------------------------------------
        std::string barcode = "AAAAAAAAAAAAAAAAAAAAAAAA";

        for( int b = 0; b <= 24; ++b )
        {
            std::vector<int32_t> msp_freq( SusceptibilityMalariaConfig::falciparumMSPVars, 0 );
            std::vector<int32_t> pfemp1_freq( SusceptibilityMalariaConfig::falciparumPfEMP1Vars, 0 );
            for( int s = 0; s < 1000; ++s )
            {
                ParasiteGenome pg = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode, std::string(""), std::string("") );
                msp_freq[ pg.GetMSP() ] += 1;
                std::vector<int32_t> pfemp1_values = pg.GetPfEMP1EpitopesMajor();
                for( auto val : pfemp1_values )
                {
                    pfemp1_freq[ val ] += 1;
                }
            }
            printf( "%s", barcode.c_str() );
            for( auto msp : msp_freq )
            {
                printf( ",%d", msp );
            }
            printf( "\n" );
            //printf( "%s", barcode.c_str() );
            //for( auto pfemp1 : pfemp1_freq )
            //{
            //    printf( ",%d", pfemp1 );
            //}
            //printf( "\n" );
            if( b < barcode.length() )
                barcode[ b ] = 'T';
        }
    }
#endif

    TEST_FIXTURE( ParasiteFixture, TestHashCollisions )
    {
        PSEUDO_DES rng( 42 );

        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ParasiteGeneticsTest/TestHashCollisions.json" );

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        for( int x = 16; x < 17; ++x )
        {
            ParasiteGenetics::CreateInstance()->ReduceGenomeMap();
            int num_bad = 0;
            int num_pos = ParasiteGenetics::GetInstance()->GetNumBasePairs();
            int num_samples = 1000000;
            for( int i_sample = 0; i_sample < num_samples; ++i_sample )
            {
                std::vector<int32_t> roots( num_pos, 0 );
                std::string barcode = "";
                for( int i_pos = 0; i_pos < num_pos; ++i_pos )
                {
                    if( rng.SmartDraw( 0.0f ) )
                    {
                        barcode += "A";
                    }
                    else
                    {
                        barcode += "C";
                    }
                    if( i_pos < x )
                    {
                        uint16_t root16 = rng.uniformZeroToN16( 2 );
                        roots[ i_pos ] = int32_t( root16 );
                    }
                }
                ParasiteGenomeInner* p_inner = ParasiteGenetics::GetInstance()->TEST_CreateGenomeInner( barcode, roots );

                if( !ParasiteGenetics::GetInstance()->CheckHashcodes( p_inner ) )
                {
                    ++num_bad;
                    delete p_inner;
                }
                else
                {
                    ParasiteGenome genome = ParasiteGenome::TEST_CreateGenome( p_inner, false );
                }
                if( (i_sample + 1) == num_samples )
                {
                    MemoryGauge mem;
                    printf( "num genomes in map=%d  num_bad=%d  ram=%d\n", ParasiteGenetics::GetInstance()->GetGenomeMapSize(), num_bad, mem.GetProcessMemory().currentMB );
                }
            }
            CHECK_EQUAL( 0, num_bad );
            CHECK_EQUAL( int(pow(2,x)), ParasiteGenetics::GetInstance()->GetGenomeMapSize() );
        }
    }

    TEST_FIXTURE( ParasiteFixture, TestIDAndNumInstances )
    {
        PSEUDO_DES rng( 42 );

        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ParasiteGeneticsTest/TestIDAndNumInstances.json" );

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        std::string barcode_1 = "AAAAAAAAAAAAAAAAAAAAAAAA";
        std::string drug_1 = "";
        std::string hrp_1 = "";
        // ----------------------------------------------------------------
        // --- Put creation of genomes in a scope that when the scope ends
        // --- the map will be only one with a handle to the genomes.
        // ----------------------------------------------------------------
        {
            // create and add one genome
            ParasiteGenome pg_tmp_1 = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode_1, drug_1, hrp_1 );
            ParasiteGenome pg_1 = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_1, 1 );

            CHECK_EQUAL( 2, pg_1.GetID() );
            CHECK_EQUAL( -8281306736165534423, pg_1.GetHashcode() );
            CHECK_EQUAL( 1, ParasiteGenetics::GetInstance()->GetGenomeMapSize() );

            // create a second, different genome and verify that the map now has the two entries
            std::string barcode_2 = barcode_1;
            ParasiteGenome pg_tmp_2 = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode_2, drug_1, hrp_1 );
            ParasiteGenome pg_2 = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_2, 2 );

            CHECK_EQUAL( 4, pg_2.GetID() );
            CHECK_EQUAL( 2, ParasiteGenetics::GetInstance()->GetGenomeMapSize() );

            // create a thrid, different genome
            std::string barcode_3 = barcode_1;
            ParasiteGenome pg_tmp_3 = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode_3, drug_1, hrp_1 );
            ParasiteGenome pg_3 = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_3, 3 );

            CHECK_EQUAL( 6, pg_3.GetID() );
            CHECK_EQUAL( 3, ParasiteGenetics::GetInstance()->GetGenomeMapSize() );

            // -----------------------------------------------------------------
            // --- Create a new genome that is the same as the first one
            // --- and verify that we just reused the pointer.  Also, verify
            // --- that this genome has the same ID and hashcode as the original.
            // --- Verify that we didn't add anything to the map
            // -----------------------------------------------------------------
            ParasiteGenome pg_tmp_1b = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode_1, drug_1, hrp_1 );
            ParasiteGenome pg_1b = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_1b, 1 );

            CHECK_EQUAL( -8281306736165534423, pg_1b.GetHashcode() );
            CHECK_EQUAL( 2, pg_1b.GetID() ); // same ID
            CHECK_EQUAL( 3, ParasiteGenetics::GetInstance()->GetGenomeMapSize() ); // nap size has not changed

            // verify that we can add a forth, different genome
            std::string barcode_4 = barcode_1;
            ParasiteGenome pg_tmp_4 = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode_4, drug_1, hrp_1 );
            ParasiteGenome pg_4 = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_4, 4 );

            CHECK_EQUAL( 10, pg_4.GetID() );
            CHECK_EQUAL( 4, ParasiteGenetics::GetInstance()->GetGenomeMapSize() );
        }

        // --------------------------------------------------------------
        // --- Clear the map and verify that it does not have any genomes
        // --------------------------------------------------------------
        ParasiteGenetics::CreateInstance()->ReduceGenomeMap();

        CHECK_EQUAL( 0, ParasiteGenetics::GetInstance()->GetGenomeMapSize() );

        // -----------------------------------------------------------------
        // --- Create a new genome and see that the map has increased by one
        // -----------------------------------------------------------------
        std::string barcode_5 = barcode_1;
        ParasiteGenome pg_tmp_5 = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode_5, drug_1, hrp_1 );
        ParasiteGenome pg_5 = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_5, 5 );

        CHECK_EQUAL( 12, pg_5.GetID() );
        CHECK_EQUAL( -6876492153842306679, pg_5.GetHashcode() );
        CHECK_EQUAL( 1, ParasiteGenetics::GetInstance()->GetGenomeMapSize() );

        // -------------------------------------------------------------------------------
        // --- Create a second genome with the same nucleotide sequence and allele roots
        // --- as a previous genome that has been cleared from the map.  We expect to see
        // --- the same ID as the old one since it has the same sequence.  The map will
        // --- increase in size since the it exists again.
        // -------------------------------------------------------------------------------
        ParasiteGenome pg_tmp_1c = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode_1, drug_1, hrp_1 );
        ParasiteGenome pg_1c = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_1c, 1 );

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! I had to remove the ability get the same ID for a genome that existed
        // !!! in the past.  It the map was using too much memory.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //CHECK_EQUAL( 2, pg_1c.GetID() ); // same ID
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        CHECK_EQUAL( -8281306736165534423, pg_1c.GetHashcode() );
        CHECK_EQUAL( 2, ParasiteGenetics::GetInstance()->GetGenomeMapSize() ); // map size has not changed

        // -------------------------------------------------------------------------
        // --- Create a genome like one of the previous two and verify that the map
        // --- doesn't change size and the we get the same ID.  i.e. didn't create
        // --- more memory.
        // -------------------------------------------------------------------------
        ParasiteGenome pg_tmp_5b = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode_5, drug_1, hrp_1 );
        ParasiteGenome pg_5b = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_5b, 5 );

        CHECK_EQUAL( 12, pg_5b.GetID() );
        CHECK_EQUAL( -6876492153842306679, pg_5b.GetHashcode() );
        CHECK_EQUAL( 2, ParasiteGenetics::GetInstance()->GetGenomeMapSize() );

        // --------------------------------------------------------------
        // --- Reduce the map and see that it has not changed size because
        // --- we still have active objects to the genomes.
        // --------------------------------------------------------------
        ParasiteGenetics::CreateInstance()->ReduceGenomeMap();

        CHECK_EQUAL( 2, ParasiteGenetics::GetInstance()->GetGenomeMapSize() );
    }

#if 1
    TEST_FIXTURE( ParasiteFixture, TestSuccessfulReadOfParameters )
    {
        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ParasiteGeneticsTest/TestSuccessfulReadOfParameters.json" );

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        CHECK_EQUAL( false, ParasiteGenetics::GetInstance()->IsFPGSimulatingBaseModel() );

        int32_t index_msp = ParasiteGenetics::GetInstance()->GetIndexMSP();
        const std::vector<int32_t>& r_indexes_barcode = ParasiteGenetics::GetInstance()->GetIndexesBarcode();
        const std::vector<int32_t>& r_indexes_drug    = ParasiteGenetics::GetInstance()->GetIndexesDrugResistant();
        const std::vector<int32_t>& r_indexes_hrp     = ParasiteGenetics::GetInstance()->GetIndexesHRP();
        const std::vector<int32_t>& r_indexes_pfemp1  = ParasiteGenetics::GetInstance()->GetIndexesPfEMP1Major();

        CHECK_EQUAL( 16, index_msp );
        CHECK_EQUAL( 24, r_indexes_barcode.size() );
        CHECK_EQUAL(  0, r_indexes_barcode[  0 ] ); // 2
        CHECK_EQUAL(  2, r_indexes_barcode[  1 ] ); // 10
        CHECK_EQUAL(  4, r_indexes_barcode[  2 ] ); // 20
        CHECK_EQUAL(  6, r_indexes_barcode[  3 ] ); // 30
        CHECK_EQUAL(  8, r_indexes_barcode[  4 ] ); // 40
        CHECK_EQUAL( 10, r_indexes_barcode[  5 ] ); // 50
        CHECK_EQUAL( 12, r_indexes_barcode[  6 ] ); // 60
        CHECK_EQUAL( 14, r_indexes_barcode[  7 ] ); // 70
        CHECK_EQUAL( 17, r_indexes_barcode[  8 ] ); // 80
        CHECK_EQUAL( 19, r_indexes_barcode[  9 ] ); // 90
        CHECK_EQUAL( 21, r_indexes_barcode[ 10 ] ); // 100
        CHECK_EQUAL( 22, r_indexes_barcode[ 11 ] ); // 110
        CHECK_EQUAL( 23, r_indexes_barcode[ 12 ] ); // 120
        CHECK_EQUAL( 24, r_indexes_barcode[ 13 ] ); // 130
        CHECK_EQUAL( 25, r_indexes_barcode[ 14 ] ); // 140
        CHECK_EQUAL( 26, r_indexes_barcode[ 15 ] ); // 150
        CHECK_EQUAL( 27, r_indexes_barcode[ 16 ] ); // 160
        CHECK_EQUAL( 28, r_indexes_barcode[ 17 ] ); // 170
        CHECK_EQUAL( 29, r_indexes_barcode[ 18 ] ); // 180
        CHECK_EQUAL( 30, r_indexes_barcode[ 19 ] ); // 190
        CHECK_EQUAL( 31, r_indexes_barcode[ 20 ] ); // 200
        CHECK_EQUAL( 32, r_indexes_barcode[ 21 ] ); // 210
        CHECK_EQUAL( 33, r_indexes_barcode[ 22 ] ); // 220
        CHECK_EQUAL( 34, r_indexes_barcode[ 23 ] ); // 230

        CHECK_EQUAL( 50, r_indexes_pfemp1.size() ); //CLONAL_PfEMP1_VARIANTS
        CHECK_EQUAL(  1, r_indexes_pfemp1[  0 ] ); // 3
        CHECK_EQUAL(  3, r_indexes_pfemp1[  1 ] ); // 11
        CHECK_EQUAL(  5, r_indexes_pfemp1[  2 ] ); // 21
        CHECK_EQUAL(  7, r_indexes_pfemp1[  3 ] ); // 31
        CHECK_EQUAL(  9, r_indexes_pfemp1[  4 ] ); // 41
        CHECK_EQUAL( 11, r_indexes_pfemp1[  5 ] ); // 51
        CHECK_EQUAL( 13, r_indexes_pfemp1[  6 ] ); // 61
        CHECK_EQUAL( 15, r_indexes_pfemp1[  7 ] ); // 71
        CHECK_EQUAL( 18, r_indexes_pfemp1[  8 ] ); // 81
        CHECK_EQUAL( 20, r_indexes_pfemp1[  9 ] ); // 91
        CHECK_EQUAL( 35, r_indexes_pfemp1[ 10 ] ); // 1000
        CHECK_EQUAL( 36, r_indexes_pfemp1[ 11 ] ); // 1001
        CHECK_EQUAL( 37, r_indexes_pfemp1[ 12 ] ); // 1002
        CHECK_EQUAL( 38, r_indexes_pfemp1[ 13 ] ); // 1003
        CHECK_EQUAL( 39, r_indexes_pfemp1[ 14 ] ); // 1004
        CHECK_EQUAL( 40, r_indexes_pfemp1[ 15 ] ); // 1005
        CHECK_EQUAL( 41, r_indexes_pfemp1[ 16 ] ); // 1006
        CHECK_EQUAL( 42, r_indexes_pfemp1[ 17 ] ); // 1007
        CHECK_EQUAL( 43, r_indexes_pfemp1[ 18 ] ); // 1008
        CHECK_EQUAL( 44, r_indexes_pfemp1[ 19 ] ); // 1009
        CHECK_EQUAL( 45, r_indexes_pfemp1[ 20 ] ); // 1100
        CHECK_EQUAL( 46, r_indexes_pfemp1[ 21 ] ); // 1101
        CHECK_EQUAL( 47, r_indexes_pfemp1[ 22 ] ); // 1102
        CHECK_EQUAL( 48, r_indexes_pfemp1[ 23 ] ); // 1103
        CHECK_EQUAL( 49, r_indexes_pfemp1[ 24 ] ); // 1104
        CHECK_EQUAL( 50, r_indexes_pfemp1[ 25 ] ); // 1105
        CHECK_EQUAL( 51, r_indexes_pfemp1[ 26 ] ); // 1106
        CHECK_EQUAL( 52, r_indexes_pfemp1[ 27 ] ); // 1107
        CHECK_EQUAL( 53, r_indexes_pfemp1[ 28 ] ); // 1108
        CHECK_EQUAL( 54, r_indexes_pfemp1[ 29 ] ); // 1109
        CHECK_EQUAL( 55, r_indexes_pfemp1[ 30 ] ); // 1200
        CHECK_EQUAL( 56, r_indexes_pfemp1[ 31 ] ); // 1201
        CHECK_EQUAL( 57, r_indexes_pfemp1[ 32 ] ); // 1202
        CHECK_EQUAL( 58, r_indexes_pfemp1[ 33 ] ); // 1203
        CHECK_EQUAL( 59, r_indexes_pfemp1[ 34 ] ); // 1204
        CHECK_EQUAL( 60, r_indexes_pfemp1[ 35 ] ); // 1205
        CHECK_EQUAL( 61, r_indexes_pfemp1[ 36 ] ); // 1206
        CHECK_EQUAL( 62, r_indexes_pfemp1[ 37 ] ); // 1207
        CHECK_EQUAL( 63, r_indexes_pfemp1[ 38 ] ); // 1208
        CHECK_EQUAL( 64, r_indexes_pfemp1[ 39 ] ); // 1209
        CHECK_EQUAL( 65, r_indexes_pfemp1[ 40 ] ); // 1300
        CHECK_EQUAL( 66, r_indexes_pfemp1[ 41 ] ); // 1301
        CHECK_EQUAL( 67, r_indexes_pfemp1[ 42 ] ); // 1302
        CHECK_EQUAL( 68, r_indexes_pfemp1[ 43 ] ); // 1303
        CHECK_EQUAL( 69, r_indexes_pfemp1[ 44 ] ); // 1304
        CHECK_EQUAL( 70, r_indexes_pfemp1[ 45 ] ); // 1305
        CHECK_EQUAL( 71, r_indexes_pfemp1[ 46 ] ); // 1306
        CHECK_EQUAL( 72, r_indexes_pfemp1[ 47 ] ); // 1307
        CHECK_EQUAL( 73, r_indexes_pfemp1[ 48 ] ); // 1308
        CHECK_EQUAL( 74, r_indexes_pfemp1[ 49 ] ); // 1309

        CHECK_EQUAL( 75, r_indexes_drug[ 0 ] ); // 2000
        CHECK_EQUAL( 76, r_indexes_drug[ 1 ] ); // 2001
        CHECK_EQUAL( 77, r_indexes_drug[ 2 ] ); // 2002


        CHECK_EQUAL( 78, r_indexes_hrp[ 0 ] ); // 3000
        CHECK_EQUAL( 79, r_indexes_hrp[ 1 ] ); // 3001
    }

    TEST_FIXTURE( ParasiteFixture, TestCreateGenomeFromBarcode )
    {
        PSEUDO_DES rng( 42 );

        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ParasiteGeneticsTest/TestCreateGenomeFromBarcode.json" );

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        // ------------------------------------------------
        // --- Test the creation of a genome from a barcode
        // ------------------------------------------------
        std::string barcode_1 = "ACGTTGCAACGTTGCAACGTTGCA";
        std::string drug_1 = "";
        std::string hrp_1 = "";
        ParasiteGenome pg_1 = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode_1, drug_1, hrp_1 );

        //printf( "%lld\n", uint64_t(pg_1.GetID()) );
        CHECK_EQUAL(          1, pg_1.GetID() );
        CHECK_EQUAL(         63, pg_1.GetMSP() );
        CHECK_EQUAL(  barcode_1, pg_1.GetBarcode() );
        CHECK_EQUAL(     drug_1, pg_1.GetDrugResistantString() );
        CHECK_EQUAL(      hrp_1, pg_1.GetHrpString() );

        CHECK( (0 <= pg_1.GetMSP()) && (pg_1.GetMSP() <= SusceptibilityMalariaConfig::falciparumMSPVars) );

        std::vector<int32_t> pfemp1_values1 = pg_1.GetPfEMP1EpitopesMajor();
        CHECK_EQUAL( 50, pfemp1_values1.size() ); //CLONAL_PfEMP1_VARIANTS
        for( auto val : pfemp1_values1 )
        {
            CHECK( (0 <= val) && (val <= SusceptibilityMalariaConfig::falciparumPfEMP1Vars) );
        }

        // ------------------------------------------------
        // --- Create another genome with the same barcode
        // --- 1) The barcode should be the same
        // --- 2) The MSP variant should be in the neighboorhood of the first
        // --- 3) All of the PfEMP1 variants should be in the neighboorhood of the first
        // ------------------------------------------------
        ParasiteGenome pg_2 = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode_1, drug_1, hrp_1 );

        CHECK_EQUAL( pg_1.GetBarcode(),             pg_2.GetBarcode() );
        CHECK_EQUAL( pg_1.GetDrugResistantString(), pg_2.GetDrugResistantString() );
        CHECK_EQUAL( pg_1.GetHrpString(),           pg_2.GetHrpString() );
        CHECK_CLOSE( pg_1.GetMSP(),pg_2.GetMSP(), ParasiteGenetics::GetInstance()->GetNeighborhoodSizeMSP() );

        CHECK( (0 <=pg_2.GetMSP()) && (pg_2.GetMSP() <= SusceptibilityMalariaConfig::falciparumMSPVars) );

        std::vector<int32_t> pfemp1_values2 = pg_2.GetPfEMP1EpitopesMajor();
        CHECK_EQUAL( pfemp1_values1.size(), pfemp1_values2.size() );
        for( int i = 0; i < pfemp1_values1.size(); ++i )
        {
            CHECK( (0 <= pfemp1_values2[ i ]) && (pfemp1_values2[ i ] <= SusceptibilityMalariaConfig::falciparumPfEMP1Vars) );
            CHECK_CLOSE( pfemp1_values1[ i ], pfemp1_values2[ i ], ParasiteGenetics::GetInstance()->GetNeighborhoodSizePfEMP1Major() );
        }

        int32_t num_samples = 20000;
        int32_t num_same_msp = 0;
        std::vector<int32_t> num_pfemp1_same( 50, 0 );
        int32_t num_variants_the_same = 0;
        for( int i_sample = 0; i_sample < num_samples; ++i_sample )
        {
            ParasiteGenome pg_sample = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode_1, drug_1, hrp_1 );

            CHECK_EQUAL( pg_1.GetBarcode(),             pg_sample.GetBarcode() );
            CHECK_EQUAL( pg_1.GetDrugResistantString(), pg_sample.GetDrugResistantString() );
            CHECK_EQUAL( pg_1.GetHrpString(),           pg_sample.GetHrpString() );
            CHECK_CLOSE( pg_1.GetMSP(), pg_sample.GetMSP(), ParasiteGenetics::GetInstance()->GetNeighborhoodSizeMSP() );
            if( pg_1.GetMSP() == pg_sample.GetMSP() ) ++num_same_msp;

            std::vector<int32_t> pfemp1_values_sample = pg_sample.GetPfEMP1EpitopesMajor();
            CHECK_EQUAL( pfemp1_values1.size(), pfemp1_values_sample.size() );
            for( int i = 0; i < pfemp1_values1.size(); ++i )
            {
                CHECK( (0 <= pfemp1_values_sample[ i ]) && (pfemp1_values_sample[ i ] <= SusceptibilityMalariaConfig::falciparumPfEMP1Vars) );
                CHECK_CLOSE( pfemp1_values1[ i ], pfemp1_values_sample[ i ], ParasiteGenetics::GetInstance()->GetNeighborhoodSizePfEMP1Major() );
                if( pfemp1_values1[ i ] == pfemp1_values_sample[ i ] )
                {
                    num_pfemp1_same[ i ] += 1;
                    num_variants_the_same += 1;
                }
            }
        }
        float fraction_same_msp_expected = 1.0 / float( ParasiteGenetics::GetInstance()->GetNeighborhoodSizeMSP() );
        float fraction_same_msp_actual = float( num_same_msp ) / num_samples;
        CHECK_CLOSE( fraction_same_msp_expected, fraction_same_msp_actual, 0.005 );

        float fraction_same_pfemp1_expected = 1.0 / float( ParasiteGenetics::GetInstance()->GetNeighborhoodSizePfEMP1Major() );
        for( auto pfemp1_same : num_pfemp1_same )
        {
            float fraction_same_pfemp1_actual = float( pfemp1_same ) / num_samples;
            CHECK_CLOSE( fraction_same_pfemp1_expected, fraction_same_pfemp1_actual, 0.006 );
        }

        float fraction_same_variants_per_genome_expected = 50 * fraction_same_pfemp1_expected;
        float fraction_same_variants_per_genome_actual = float( num_variants_the_same ) / float( num_samples );
        CHECK_CLOSE( fraction_same_variants_per_genome_expected, fraction_same_variants_per_genome_actual, 0.1 );

        // -------------------------------------------------------------
        // --- Create another with a barcode that is slightly different
        // --- The var genes of this genome should "mostly" be in the same
        // --- neighboorhood as our first genome.
        // -------------------------------------------------------------
        std::string barcode_3 = "ACGTTGCAACGTTGCAACGTTGCC";

        num_same_msp = 0;
        std::fill( num_pfemp1_same.begin(), num_pfemp1_same.end(), 0 );
        for( int i_sample = 0; i_sample < num_samples; ++i_sample )
        {
            ParasiteGenome pg_sample = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode_3, drug_1, hrp_1 );

            CHECK_EQUAL( barcode_3, pg_sample.GetBarcode() );
            CHECK_CLOSE( pg_1.GetMSP(), pg_sample.GetMSP(), ParasiteGenetics::GetInstance()->GetNeighborhoodSizeMSP() );
            CHECK( (0 <= pg_sample.GetMSP()) && (pg_sample.GetMSP() <= SusceptibilityMalariaConfig::falciparumMSPVars) );
            if( pg_1.GetMSP() == pg_sample.GetMSP() ) ++num_same_msp;

            std::vector<int32_t> pfemp1_values_sample = pg_sample.GetPfEMP1EpitopesMajor();
            CHECK_EQUAL( pfemp1_values1.size(), pfemp1_values_sample.size() );
            for( int i = 0; i < pfemp1_values1.size(); ++i )
            {
                CHECK( (0 <= pfemp1_values_sample[ i ]) && (pfemp1_values_sample[ i ] <= SusceptibilityMalariaConfig::falciparumPfEMP1Vars) );
                CHECK_CLOSE( pfemp1_values1[ i ], pfemp1_values_sample[ i ], ParasiteGenetics::GetInstance()->GetNeighborhoodSizePfEMP1Major() + 3 ); //mostly
                if( pfemp1_values1[ i ] == pfemp1_values_sample[ i ] )
                {
                    num_pfemp1_same[ i ] += 1;
                }
            }
        }
        fraction_same_msp_actual = float( num_same_msp ) / num_samples;
        CHECK_CLOSE( fraction_same_msp_expected, fraction_same_msp_actual, 0.005 );

        // -------------------------------------------------------------
        // --- Create another with a barcode that is different.
        // --- The var genes of this genome should NOT be in the same
        // --- neighboorhood as our first genome.
        // -------------------------------------------------------------
        std::string barcode_4 = "TTTTAAAAGGGGCCCCTTTTGGGG";

        num_same_msp = 0;
        std::fill( num_pfemp1_same.begin(), num_pfemp1_same.end(), 0 );
        for( int i_sample = 0; i_sample < num_samples; ++i_sample )
        {
            ParasiteGenome pg_sample = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode_4, drug_1, hrp_1 );

            CHECK_EQUAL( barcode_4, pg_sample.GetBarcode() );
            CHECK( (0 <= pg_sample.GetMSP()) && (pg_sample.GetMSP() <= SusceptibilityMalariaConfig::falciparumMSPVars) );

            int32_t min_msp_1 = pg_1.GetMSP() - ParasiteGenetics::GetInstance()->GetNeighborhoodSizeMSP()/2;
            int32_t max_msp_1 = pg_1.GetMSP() + ParasiteGenetics::GetInstance()->GetNeighborhoodSizeMSP()/2;
            CHECK( (pg_sample.GetMSP() < min_msp_1) || (max_msp_1 < pg_sample.GetMSP()) );

            std::vector<int32_t> pfemp1_values_sample = pg_sample.GetPfEMP1EpitopesMajor();
            CHECK_EQUAL( pfemp1_values1.size(), pfemp1_values_sample.size() );
            for( int i = 0; i < pfemp1_values1.size(); ++i )
            {
                CHECK( (0 <= pfemp1_values_sample[ i ]) && (pfemp1_values_sample[ i ] <= SusceptibilityMalariaConfig::falciparumPfEMP1Vars) );

                int32_t min_pfemp1_1 = pfemp1_values1[ i ] - ParasiteGenetics::GetInstance()->GetNeighborhoodSizePfEMP1Major()/2;
                int32_t max_pfemp1_1 = pfemp1_values1[ i ] + ParasiteGenetics::GetInstance()->GetNeighborhoodSizePfEMP1Major()/2;

                CHECK( (pfemp1_values_sample[ i ] < min_pfemp1_1) || (max_pfemp1_1 < pfemp1_values_sample[ i ]) );
            }
        }

        // ----------------------------------------------------
        // --- Test how different the barcode needs to be before
        // --- MSP is not in same neighboorhood
        // ----------------------------------------------------
        std::string barcode_5 = barcode_1;
        bool done = false;
        int index = barcode_5.size() - 1;
        while( !done )
        {
            ParasiteGenome pg_5 = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode_5, drug_1, hrp_1 );

            CHECK_EQUAL( barcode_5, pg_5.GetBarcode() );
            CHECK( (0 <= pg_5.GetMSP()) && (pg_5.GetMSP() <= SusceptibilityMalariaConfig::falciparumMSPVars) );

            int32_t min_msp_1 = pg_1.GetMSP() - ParasiteGenetics::GetInstance()->GetNeighborhoodSizeMSP()/2;
            int32_t max_msp_1 = pg_1.GetMSP() + ParasiteGenetics::GetInstance()->GetNeighborhoodSizeMSP()/2;
            if( (min_msp_1 <= pg_5.GetMSP()) && (pg_5.GetMSP() <= max_msp_1) )
            {
                barcode_5[ index ] = 'T';
                --index;
                done = (index < 0);
            }
            else
            {
                done = true;
                CHECK_EQUAL( "ACGTTGCAACGTTGCAACGTTGTT", barcode_5 );
            }
        }

        // ----------------------------------------------------
        // --- Test how different the barcode needs to be before
        // --- PfEMP1 is not in same neighboorhood
        // ----------------------------------------------------
        std::string barcode_6 = barcode_1;
        done = false;
        index = barcode_6.size() - 1;
        while( !done )
        {
            ParasiteGenome pg_6 = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode_6, drug_1, hrp_1 );

            CHECK_EQUAL( barcode_6, pg_6.GetBarcode() );
            CHECK( (0 <= pg_6.GetMSP()) && (pg_6.GetMSP() <= SusceptibilityMalariaConfig::falciparumMSPVars) );

            bool at_least_one_in_neighboorhood = false;
            std::vector<int32_t> pfemp1_values6 = pg_6.GetPfEMP1EpitopesMajor();
            CHECK_EQUAL( pfemp1_values1.size(), pfemp1_values6.size() );
            for( int i = 0; !at_least_one_in_neighboorhood && (i < pfemp1_values6.size()); ++i )
            {
                CHECK( (0 <= pfemp1_values6[ i ]) && (pfemp1_values6[ i ] <= SusceptibilityMalariaConfig::falciparumPfEMP1Vars) );

                int32_t min_pfemp1_1 = pfemp1_values1[ i ] - ParasiteGenetics::GetInstance()->GetNeighborhoodSizePfEMP1Major()/2;
                int32_t max_pfemp1_1 = pfemp1_values1[ i ] + ParasiteGenetics::GetInstance()->GetNeighborhoodSizePfEMP1Major()/2;

                int32_t pfemp1_6 = pfemp1_values6[ i ];

                at_least_one_in_neighboorhood |= (min_pfemp1_1 <= pfemp1_6) && (pfemp1_6 <= max_pfemp1_1);
            }

            if( at_least_one_in_neighboorhood )
            {
                barcode_6[ index ] = 'T';
                --index;
                done = (index < 0);
            }
            else
            {
                done = true;
                CHECK_EQUAL( "ACGTTGCAACGTTGCAACGTTGCT", barcode_6 );
            }
        }
    }

    TEST_FIXTURE( ParasiteFixture, TestCreateGenomeFromSequence )
    {
        // --------------------------------
        // --- Also test reference tracking
        // --------------------------------
        CHECK_EQUAL( 4, ParasiteGenome::GetNumActive() );

        PSEUDO_DES rng( 42 );

        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ParasiteGeneticsTest/TestCreateGenomeFromSequence.json" );

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        // ------------------------------------------------
        // --- Test the creation of a genome from a barcode
        // --- and a set of var genes
        // ------------------------------------------------
        std::string barcode = "ACGTTGCAACGTTGCAACGTTGCA";
        std::string drug = "";
        std::string hrp = "";
        int32_t msp = 16;
        std::vector<int32_t> pfemp1_values = {
             1,  2,  3,  4,  5,  6,  7,  8,  9, 10,
            11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
            21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
            31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
            41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
        };
        ParasiteGenome pg_1 = ParasiteGenetics::GetInstance()->CreateGenomeFromSequence( &rng, barcode, drug, hrp, msp, pfemp1_values );

        CHECK_EQUAL(          1, pg_1.GetID() );
        CHECK_EQUAL(        msp, pg_1.GetMSP() );
        CHECK_EQUAL(    barcode, pg_1.GetBarcode() );

        std::vector<int32_t> pfemp1_values_actual = pg_1.GetPfEMP1EpitopesMajor();
        CHECK_EQUAL( 50, pfemp1_values_actual.size() ); //CLONAL_PfEMP1_VARIANTS
        CHECK_EQUAL(  1, pfemp1_values_actual[  0 ] );
        CHECK_EQUAL(  2, pfemp1_values_actual[  1 ] );
        CHECK_EQUAL(  3, pfemp1_values_actual[  2 ] );
        CHECK_EQUAL(  4, pfemp1_values_actual[  3 ] );
        CHECK_EQUAL(  5, pfemp1_values_actual[  4 ] );
        CHECK_EQUAL(  6, pfemp1_values_actual[  5 ] );
        CHECK_EQUAL(  7, pfemp1_values_actual[  6 ] );
        CHECK_EQUAL(  8, pfemp1_values_actual[  7 ] );
        CHECK_EQUAL(  9, pfemp1_values_actual[  8 ] );
        CHECK_EQUAL( 10, pfemp1_values_actual[  9 ] );
        CHECK_EQUAL( 11, pfemp1_values_actual[ 10 ] );
        CHECK_EQUAL( 12, pfemp1_values_actual[ 11 ] );
        CHECK_EQUAL( 13, pfemp1_values_actual[ 12 ] );
        CHECK_EQUAL( 14, pfemp1_values_actual[ 13 ] );
        CHECK_EQUAL( 15, pfemp1_values_actual[ 14 ] );
        CHECK_EQUAL( 16, pfemp1_values_actual[ 15 ] );
        CHECK_EQUAL( 17, pfemp1_values_actual[ 16 ] );
        CHECK_EQUAL( 18, pfemp1_values_actual[ 17 ] );
        CHECK_EQUAL( 19, pfemp1_values_actual[ 18 ] );
        CHECK_EQUAL( 20, pfemp1_values_actual[ 19 ] );
        CHECK_EQUAL( 21, pfemp1_values_actual[ 20 ] );
        CHECK_EQUAL( 22, pfemp1_values_actual[ 21 ] );
        CHECK_EQUAL( 23, pfemp1_values_actual[ 22 ] );
        CHECK_EQUAL( 24, pfemp1_values_actual[ 23 ] );
        CHECK_EQUAL( 25, pfemp1_values_actual[ 24 ] );
        CHECK_EQUAL( 26, pfemp1_values_actual[ 25 ] );
        CHECK_EQUAL( 27, pfemp1_values_actual[ 26 ] );
        CHECK_EQUAL( 28, pfemp1_values_actual[ 27 ] );
        CHECK_EQUAL( 29, pfemp1_values_actual[ 28 ] );
        CHECK_EQUAL( 30, pfemp1_values_actual[ 29 ] );
        CHECK_EQUAL( 31, pfemp1_values_actual[ 30 ] );
        CHECK_EQUAL( 32, pfemp1_values_actual[ 31 ] );
        CHECK_EQUAL( 33, pfemp1_values_actual[ 32 ] );
        CHECK_EQUAL( 34, pfemp1_values_actual[ 33 ] );
        CHECK_EQUAL( 35, pfemp1_values_actual[ 34 ] );
        CHECK_EQUAL( 36, pfemp1_values_actual[ 35 ] );
        CHECK_EQUAL( 37, pfemp1_values_actual[ 36 ] );
        CHECK_EQUAL( 38, pfemp1_values_actual[ 37 ] );
        CHECK_EQUAL( 39, pfemp1_values_actual[ 38 ] );
        CHECK_EQUAL( 40, pfemp1_values_actual[ 39 ] );
        CHECK_EQUAL( 41, pfemp1_values_actual[ 40 ] );
        CHECK_EQUAL( 42, pfemp1_values_actual[ 41 ] );
        CHECK_EQUAL( 43, pfemp1_values_actual[ 42 ] );
        CHECK_EQUAL( 44, pfemp1_values_actual[ 43 ] );
        CHECK_EQUAL( 45, pfemp1_values_actual[ 44 ] );
        CHECK_EQUAL( 46, pfemp1_values_actual[ 45 ] );
        CHECK_EQUAL( 47, pfemp1_values_actual[ 46 ] );
        CHECK_EQUAL( 48, pfemp1_values_actual[ 47 ] );
        CHECK_EQUAL( 49, pfemp1_values_actual[ 48 ] );
        CHECK_EQUAL( 50, pfemp1_values_actual[ 49 ] );

        // ----------------------------
        // --- Test reference tracking
        // ----------------------------

        // --------------------------------------------------
        // --- 5 comes from the 4 statics plus the one above
        // --------------------------------------------------
        CHECK_EQUAL( 5, ParasiteGenome::GetNumActive() );

        // put the tests inside a scope so that they are deleted on leaving the scope
        if( ParasiteGenome::GetNumActive() == 5 )
        {
            ParasiteGenome pg_2 = ParasiteGenetics::GetInstance()->CreateGenomeFromSequence( &rng, barcode, drug, hrp, msp, pfemp1_values );

            // 6 = 4 statics + pg_1 & pg_2
            CHECK_EQUAL( 6, ParasiteGenome::GetNumActive() );
            CHECK_EQUAL( pg_1.GetHashcode(), pg_2.GetHashcode() );

            barcode[ 0 ] = 'T';

            ParasiteGenome pg_3 = ParasiteGenetics::GetInstance()->CreateGenomeFromSequence( &rng, barcode, drug, hrp, msp, pfemp1_values );

            // 6 = 4 statics + pg_1 & pg_2 & pg_3
            CHECK_EQUAL( 7, ParasiteGenome::GetNumActive() );
            CHECK( pg_1.GetHashcode() != pg_3.GetHashcode() );

            // create an object with no data
            ParasiteGenome pg_3b;
            CHECK( pg_3b.IsNull() );
            CHECK_EQUAL( 7, ParasiteGenome::GetNumActive() );

            // assign that no data object to one that has data
            // we shouldn't see the number of data objects increase
            pg_3b = pg_3;
            CHECK( !pg_3b.IsNull() );
            CHECK_EQUAL( pg_3.GetHashcode(), pg_3b.GetHashcode() );
            CHECK_EQUAL( 7, ParasiteGenome::GetNumActive() );

            // test the copy constructor - no increase in data objects
            ParasiteGenome pg_2b( pg_2 );
            CHECK( !pg_2b.IsNull() );
            CHECK_EQUAL( pg_2.GetHashcode(), pg_2b.GetHashcode() );
            CHECK_EQUAL( 7, ParasiteGenome::GetNumActive() );

            // create an object with no data
            // set pg_3 equal to that no data
            // see that by setting pg_3b we remove the data originally created for pg_3
            ParasiteGenome pg_4;
            CHECK( pg_4.IsNull() );
            CHECK_EQUAL( 7, ParasiteGenome::GetNumActive() );
            pg_3 = pg_4;
            CHECK( pg_3.IsNull() );
            CHECK( pg_4.IsNull() );
            CHECK_EQUAL( 7, ParasiteGenome::GetNumActive() );
            pg_3b = pg_4;
            CHECK( pg_3.IsNull() );
            CHECK( pg_3b.IsNull() );
            CHECK( pg_4.IsNull() );
            CHECK_EQUAL( 6, ParasiteGenome::GetNumActive() );
        }
        // All of the genomes created in the scope should have been deleted
        // 5 = 4 statis + pg_1
        CHECK_EQUAL( 5, ParasiteGenome::GetNumActive() );
    }

    TEST_FIXTURE( ParasiteFixture, TestCreateGenomeFromAlleleFrequencies )
    {
        PSEUDO_DES rng( 42 );

        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ParasiteGeneticsTest/TestCreateGenomeFromAlleleFrequencies.json" );

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        std::vector<std::vector<float>> exp_allele_freqs_barcode;
        std::vector<std::vector<float>> exp_allele_freqs_drug;
        std::vector<std::vector<float>> exp_allele_freqs_hrp;
        exp_allele_freqs_barcode.push_back( std::vector<float>( { 1.00f, 0.00f, 0.00f, 0.00f } ) );

        exp_allele_freqs_drug.push_back( std::vector<float>( { 1.00f, 0.00f, 0.00f, 0.00f } ) );
        exp_allele_freqs_drug.push_back( std::vector<float>( { 0.00f, 0.00f, 0.75f, 0.25f } ) );
        exp_allele_freqs_drug.push_back( std::vector<float>( { 0.00f, 0.90f, 0.00f, 0.10f } ) );

        exp_allele_freqs_hrp.push_back( std::vector<float>( { 0.50f, 0.50f, 0.00f, 0.00f } ) );
        exp_allele_freqs_hrp.push_back( std::vector<float>( { 0.50f, 0.50f, 0.00f, 0.00f } ) );

        // --------------------------------------------------------------------------
        // --- Test that the number of sets of frequencies does not match the number
        // --- of genome locations
        // --------------------------------------------------------------------------
        try
        {
            ParasiteGenome genome = ParasiteGenetics::GetInstance()->CreateGenomeFromAlleleFrequencies( &rng,
                                                                                                        exp_allele_freqs_barcode,
                                                                                                        exp_allele_freqs_drug,
                                                                                                        exp_allele_freqs_hrp );
        }
        catch( DetailedException& re )
        {
            std::string exp_msg;
            exp_msg += "Invalid number of frequency sets in 'Barcode_Allele_Frequencies_Per_Genome_Location'.\n";
            exp_msg += "'Barcode_Allele_Frequencies_Per_Genome_Location' has 1\n";
            exp_msg += "and 'Barcode_Genome_Locations' has 10.\n";
            exp_msg += "There should be one set for each location defined in 'Barcode_Genome_Locations'.\n";
            std::string act_msg = re.GetMsg();
            if( act_msg.find( exp_msg ) == string::npos )
            {
                PrintDebug( exp_msg );
                PrintDebug( act_msg );
                CHECK( false );
            }
        }

        exp_allele_freqs_barcode.push_back( std::vector<float>( { 0.00f, 1.00f, 0.00f, 0.00f } ) );
        exp_allele_freqs_barcode.push_back( std::vector<float>( { 0.00f, 0.00f, 1.00f, 0.00f } ) );
        exp_allele_freqs_barcode.push_back( std::vector<float>( { 0.00f, 0.00f, 0.00f, 1.00f } ) );
        exp_allele_freqs_barcode.push_back( std::vector<float>( { 0.50f, 0.50f, 0.00f, 0.00f } ) );
        exp_allele_freqs_barcode.push_back( std::vector<float>( { 0.00f, 0.50f, 0.50f, 0.00f } ) );
        exp_allele_freqs_barcode.push_back( std::vector<float>( { 0.00f, 0.00f, 0.50f, 0.50f } ) );
        exp_allele_freqs_barcode.push_back( std::vector<float>( { 0.25f, 0.25f, 0.25f, 0.25f } ) );
        exp_allele_freqs_barcode.push_back( std::vector<float>( { 0.10f, 0.20f, 0.30f, 0.40f } ) );

        // -------------------------------------------------------------------------------
        // --- Now test that we have the correct number of sets, but one of the sets does
        // --- not have 4 values - one for each possible allele
        // -------------------------------------------------------------------------------
        exp_allele_freqs_barcode.push_back( std::vector<float>( { 0.40f, 0.30f } ) );
        try
        {
            ParasiteGenome genome = ParasiteGenetics::GetInstance()->CreateGenomeFromAlleleFrequencies( &rng,
                                                                                                        exp_allele_freqs_barcode,
                                                                                                        exp_allele_freqs_drug,
                                                                                                        exp_allele_freqs_hrp );
        }
        catch( DetailedException& re )
        {
            std::string exp_msg;
            exp_msg += "Invalid number of values in frequency set number 9 in 'Barcode_Allele_Frequencies_Per_Genome_Location'.\n";
            exp_msg += "'Barcode_Allele_Frequencies_Per_Genome_Location[9]' has 2 frequencies.\n";
            exp_msg += "Each set of frequencies must have four values - one for each possible allele.";
            std::string act_msg = re.GetMsg();
            if( act_msg.find( exp_msg ) == string::npos )
            {
                PrintDebug( exp_msg );
                PrintDebug( act_msg );
                CHECK( false );
            }
        }
        exp_allele_freqs_barcode.pop_back();

        // -----------------------------------------------------------------
        // --- Now test that we have the correct number of sets and values, 
        // --- but one of the sets of values does not sum to 1
        // -----------------------------------------------------------------
        exp_allele_freqs_barcode.push_back( std::vector<float>( { 0.50f, 0.50f, 0.50f, 0.50f } ) );
        try
        {
            ParasiteGenome genome = ParasiteGenetics::GetInstance()->CreateGenomeFromAlleleFrequencies( &rng,
                                                                                                        exp_allele_freqs_barcode,
                                                                                                        exp_allele_freqs_drug,
                                                                                                        exp_allele_freqs_hrp );
        }
        catch( DetailedException& re )
        {
            std::string exp_msg;
            exp_msg += "Invalid frequency set number 9 in 'Barcode_Allele_Frequencies_Per_Genome_Location'.\n";
            exp_msg += "The values of 'Barcode_Allele_Frequencies_Per_Genome_Location[9]' sum to 2\n";
            exp_msg += "when they need to sum to 1.0.";
            std::string act_msg = re.GetMsg();
            if( act_msg.find( exp_msg ) == string::npos )
            {
                PrintDebug( exp_msg );
                PrintDebug( act_msg );
                CHECK( false );
            }
        }
        exp_allele_freqs_barcode.pop_back();
        exp_allele_freqs_barcode.push_back( std::vector<float>( { 0.40f, 0.30f, 0.20f, 0.10f } ) );

        // --------------------------------------------------------------------------
        // --- Test that the number of sets of frequencies does not match the number
        // --- of genome locations - DRUG RESISTANT
        // --------------------------------------------------------------------------
        exp_allele_freqs_drug.pop_back();
        try
        {
            ParasiteGenome genome = ParasiteGenetics::GetInstance()->CreateGenomeFromAlleleFrequencies( &rng,
                                                                                                        exp_allele_freqs_barcode,
                                                                                                        exp_allele_freqs_drug,
                                                                                                        exp_allele_freqs_hrp );
        }
        catch( DetailedException& re )
        {
            std::string exp_msg;
            exp_msg += "Invalid number of frequency sets in 'Drug_Resistant_Allele_Frequencies_Per_Genome_Location'.\n";
            exp_msg += "'Drug_Resistant_Allele_Frequencies_Per_Genome_Location' has 2\n";
            exp_msg += "and 'Drug_Resistant_Genome_Locations' has 3.\n";
            exp_msg += "There should be one set for each location defined in 'Drug_Resistant_Genome_Locations'.\n";
            std::string act_msg = re.GetMsg();
            if( act_msg.find( exp_msg ) == string::npos )
            {
                PrintDebug( exp_msg );
                PrintDebug( act_msg );
                CHECK( false );
            }
        }

        // -------------------------------------------------------------------------------
        // --- Now test that we have the correct number of sets, but one of the sets does
        // --- not have 4 values - one for each possible allele - DRUG RESISTANT
        // -------------------------------------------------------------------------------
        exp_allele_freqs_drug.push_back( std::vector<float>( { 0.40f, 0.30f } ) );
        try
        {
            ParasiteGenome genome = ParasiteGenetics::GetInstance()->CreateGenomeFromAlleleFrequencies( &rng,
                                                                                                        exp_allele_freqs_barcode,
                                                                                                        exp_allele_freqs_drug,
                                                                                                        exp_allele_freqs_hrp );
        }
        catch( DetailedException& re )
        {
            std::string exp_msg;
            exp_msg += "Invalid number of values in frequency set number 2 in 'Drug_Resistant_Allele_Frequencies_Per_Genome_Location'.\n";
            exp_msg += "'Drug_Resistant_Allele_Frequencies_Per_Genome_Location[2]' has 2 frequencies.\n";
            exp_msg += "Each set of frequencies must have four values - one for each possible allele.";
            std::string act_msg = re.GetMsg();
            if( act_msg.find( exp_msg ) == string::npos )
            {
                PrintDebug( exp_msg );
                PrintDebug( act_msg );
                CHECK( false );
            }
        }
        exp_allele_freqs_drug.pop_back();

        // -----------------------------------------------------------------
        // --- Now test that we have the correct number of sets and values, 
        // --- but one of the sets of values does not sum to 1 - DRUG RESISTANT
        // -----------------------------------------------------------------
        exp_allele_freqs_drug.push_back( std::vector<float>( { 0.50f, 0.50f, 0.50f, 0.50f } ) );
        try
        {
            ParasiteGenome genome = ParasiteGenetics::GetInstance()->CreateGenomeFromAlleleFrequencies( &rng,
                                                                                                        exp_allele_freqs_barcode,
                                                                                                        exp_allele_freqs_drug,
                                                                                                        exp_allele_freqs_hrp );
        }
        catch( DetailedException& re )
        {
            std::string exp_msg;
            exp_msg += "Invalid frequency set number 2 in 'Drug_Resistant_Allele_Frequencies_Per_Genome_Location'.\n";
            exp_msg += "The values of 'Drug_Resistant_Allele_Frequencies_Per_Genome_Location[2]' sum to 2\n";
            exp_msg += "when they need to sum to 1.0.";
            std::string act_msg = re.GetMsg();
            if( act_msg.find( exp_msg ) == string::npos )
            {
                PrintDebug( exp_msg );
                PrintDebug( act_msg );
                CHECK( false );
            }
        }
        exp_allele_freqs_drug.pop_back();
        exp_allele_freqs_drug.push_back( std::vector<float>( { 0.40f, 0.30f, 0.20f, 0.10f } ) );

        // --------------------------------------------------------------------------
        // --- Test that the number of sets of frequencies does not match the number
        // --- of genome locations - HRP
        // --------------------------------------------------------------------------
        exp_allele_freqs_hrp.pop_back();
        try
        {
            ParasiteGenome genome = ParasiteGenetics::GetInstance()->CreateGenomeFromAlleleFrequencies( &rng,
                                                                                                        exp_allele_freqs_barcode,
                                                                                                        exp_allele_freqs_drug,
                                                                                                        exp_allele_freqs_hrp );
        }
        catch( DetailedException& re )
        {
            std::string exp_msg;
            exp_msg += "Invalid number of frequency sets in 'HRP_Allele_Frequencies_Per_Genome_Location'.\n";
            exp_msg += "'HRP_Allele_Frequencies_Per_Genome_Location' has 1\n";
            exp_msg += "and 'HRP_Genome_Locations' has 2.\n";
            exp_msg += "There should be one set for each location defined in 'HRP_Genome_Locations'.\n";
            std::string act_msg = re.GetMsg();
            if( act_msg.find( exp_msg ) == string::npos )
            {
                PrintDebug( exp_msg );
                PrintDebug( act_msg );
                CHECK( false );
            }
        }

        // -------------------------------------------------------------------------------
        // --- Now test that we have the correct number of sets, but one of the sets does
        // --- not have 4 values - one for each possible allele - HRP
        // -------------------------------------------------------------------------------
        exp_allele_freqs_hrp.push_back( std::vector<float>( { 0.40f, 0.30f } ) );
        try
        {
            ParasiteGenome genome = ParasiteGenetics::GetInstance()->CreateGenomeFromAlleleFrequencies( &rng,
                                                                                                        exp_allele_freqs_barcode,
                                                                                                        exp_allele_freqs_drug,
                                                                                                        exp_allele_freqs_hrp );
        }
        catch( DetailedException& re )
        {
            std::string exp_msg;
            exp_msg += "Invalid number of values in frequency set number 1 in 'HRP_Allele_Frequencies_Per_Genome_Location'.\n";
            exp_msg += "'HRP_Allele_Frequencies_Per_Genome_Location[1]' has 2 frequencies.\n";
            exp_msg += "Each set of frequencies must have four values - one for each possible allele.";
            std::string act_msg = re.GetMsg();
            if( act_msg.find( exp_msg ) == string::npos )
            {
                PrintDebug( exp_msg );
                PrintDebug( act_msg );
                CHECK( false );
            }
        }
        exp_allele_freqs_hrp.pop_back();

        // -----------------------------------------------------------------
        // --- Now test that we have the correct number of sets and values, 
        // --- but one of the sets of values does not sum to 1 - HRP
        // -----------------------------------------------------------------
        exp_allele_freqs_hrp.push_back( std::vector<float>( { 0.50f, 0.50f, 0.50f, 0.50f } ) );
        try
        {
            ParasiteGenome genome = ParasiteGenetics::GetInstance()->CreateGenomeFromAlleleFrequencies( &rng,
                                                                                                        exp_allele_freqs_barcode,
                                                                                                        exp_allele_freqs_drug,
                                                                                                        exp_allele_freqs_hrp );
        }
        catch( DetailedException& re )
        {
            std::string exp_msg;
            exp_msg += "Invalid frequency set number 1 in 'HRP_Allele_Frequencies_Per_Genome_Location'.\n";
            exp_msg += "The values of 'HRP_Allele_Frequencies_Per_Genome_Location[1]' sum to 2\n";
            exp_msg += "when they need to sum to 1.0.";
            std::string act_msg = re.GetMsg();
            if( act_msg.find( exp_msg ) == string::npos )
            {
                PrintDebug( exp_msg );
                PrintDebug( act_msg );
                CHECK( false );
            }
        }
        exp_allele_freqs_hrp.pop_back();
        exp_allele_freqs_hrp.push_back( std::vector<float>( { 0.50f, 0.50f, 0.00f, 0.00f } ) );

        // ----------------------------------------------------------------------------------------------
        // --- Finally, let's test that we get genomes who's allele's appear at the expected frequencies.
        // ----------------------------------------------------------------------------------------------

        std::vector<std::vector<float>> act_allele_freqs_barcode;
        for( int i = 0; i < exp_allele_freqs_barcode.size(); ++i )
        {
            act_allele_freqs_barcode.push_back( std::vector<float>( { 0.0f, 0.0f, 0.0f, 0.0f } ) );
        }

        std::vector<std::vector<float>> act_allele_freqs_drug;
        for( int i = 0; i < exp_allele_freqs_drug.size(); ++i )
        {
            act_allele_freqs_drug.push_back( std::vector<float>( { 0.0f, 0.0f, 0.0f, 0.0f } ) );
        }

        std::vector<std::vector<float>> act_allele_freqs_hrp;
        for( int i = 0; i < exp_allele_freqs_hrp.size(); ++i )
        {
            act_allele_freqs_hrp.push_back( std::vector<float>( { 0.0f, 0.0f, 0.0f, 0.0f } ) );
        }

        // if we have 1M samples, then epsilon can be 0.001, but it takes longer
        int num_samples = 100000;
        for( int i = 0; i < num_samples; ++i )
        {
            ParasiteGenome genome = ParasiteGenetics::GetInstance()->CreateGenomeFromAlleleFrequencies( &rng,
                                                                                                        exp_allele_freqs_barcode,
                                                                                                        exp_allele_freqs_drug,
                                                                                                        exp_allele_freqs_hrp );
            std::string barcode = genome.GetBarcode();
            std::string drug    = genome.GetDrugResistantString();
            std::string hrp     = genome.GetHrpString();

            CHECK_EQUAL( 10, barcode.length() );
            CHECK_EQUAL(  3, drug.length() );
            CHECK_EQUAL(  2, hrp.length() );

            for( int j = 0; j < barcode.length(); ++j )
            {
                char c = barcode[ j ];
                int val = ParasiteGenetics::ConvertCharToVal( "XXX", false, c );
                act_allele_freqs_barcode[ j ][ val ] += 1.0;
            }

            for( int j = 0; j < drug.length(); ++j )
            {
                char c = drug[ j ];
                int val = ParasiteGenetics::ConvertCharToVal( "XXX", false, c );
                act_allele_freqs_drug[ j ][ val ] += 1.0;
            }

            for( int j = 0; j < hrp.length(); ++j )
            {
                char c = hrp[ j ];
                int val = ParasiteGenetics::ConvertCharToVal( "XXX", false, c );
                act_allele_freqs_hrp[ j ][ val ] += 1.0;
            }
        }

        for( int i = 0; i < act_allele_freqs_barcode.size(); ++i )
        {
            for( int j = 0; j < act_allele_freqs_barcode[ i ].size(); ++j )
            {
                act_allele_freqs_barcode[ i ][ j ] /= float( num_samples );
            }
        }

        CHECK( exp_allele_freqs_barcode.size() > 0 );
        CHECK_EQUAL( exp_allele_freqs_barcode.size(), act_allele_freqs_barcode.size() );
        for( int i = 0; i < act_allele_freqs_barcode.size(); ++i )
        {
            CHECK_EQUAL( exp_allele_freqs_barcode[i].size(), act_allele_freqs_barcode[i].size() );
        }
        CHECK_ARRAY2D_CLOSE( exp_allele_freqs_barcode, act_allele_freqs_barcode, act_allele_freqs_barcode.size(), act_allele_freqs_barcode[0].size(), 0.005 );

        for( int i = 0; i < act_allele_freqs_drug.size(); ++i )
        {
            for( int j = 0; j < act_allele_freqs_drug[ i ].size(); ++j )
            {
                act_allele_freqs_drug[ i ][ j ] /= float( num_samples );
            }
        }

        CHECK( exp_allele_freqs_drug.size() > 0 );
        CHECK_EQUAL( exp_allele_freqs_drug.size(), act_allele_freqs_drug.size() );
        for( int i = 0; i < act_allele_freqs_drug.size(); ++i )
        {
            CHECK_EQUAL( exp_allele_freqs_drug[ i ].size(), act_allele_freqs_drug[ i ].size() );
        }
        CHECK_ARRAY2D_CLOSE( exp_allele_freqs_drug, act_allele_freqs_drug, act_allele_freqs_drug.size(), act_allele_freqs_drug[ 0 ].size(), 0.005 );

        for( int i = 0; i < act_allele_freqs_hrp.size(); ++i )
        {
            for( int j = 0; j < act_allele_freqs_hrp[ i ].size(); ++j )
            {
                act_allele_freqs_hrp[ i ][ j ] /= float( num_samples );
            }
        }

        CHECK( exp_allele_freqs_hrp.size() > 0 );
        CHECK_EQUAL( exp_allele_freqs_hrp.size(), act_allele_freqs_hrp.size() );
        for( int i = 0; i < act_allele_freqs_hrp.size(); ++i )
        {
            CHECK_EQUAL( exp_allele_freqs_hrp[ i ].size(), act_allele_freqs_hrp[ i ].size() );
        }
        CHECK_ARRAY2D_CLOSE( exp_allele_freqs_hrp, act_allele_freqs_hrp, act_allele_freqs_hrp.size(), act_allele_freqs_hrp[ 0 ].size(), 0.005 );
    }

    TEST_FIXTURE( ParasiteFixture, TestFindChromosome )
    {
        CHECK_EQUAL( 0, ParasiteGenetics::FindChromosome( 0 ) );
        CHECK_EQUAL( 0, ParasiteGenetics::FindChromosome( 1 ) );
        CHECK_EQUAL( 0, ParasiteGenetics::FindChromosome( 200000 ) );
        CHECK_EQUAL( 0, ParasiteGenetics::FindChromosome( 214333 ) );
        CHECK_EQUAL( 0, ParasiteGenetics::FindChromosome( 311500 ) );
        CHECK_EQUAL( 0, ParasiteGenetics::FindChromosome( 428667 ) );
        CHECK_EQUAL( 0, ParasiteGenetics::FindChromosome( ParasiteGenetics::CHROMOSOME_LENGTH[0] ) );
        CHECK_EQUAL( 1, ParasiteGenetics::FindChromosome( ParasiteGenetics::CHROMOSOME_LENGTH[0]+1 ) );
        CHECK_EQUAL( 1, ParasiteGenetics::FindChromosome( 958667 ) );
        CHECK_EQUAL( 1, ParasiteGenetics::FindChromosome( 1116500 ) );
        CHECK_EQUAL( 1, ParasiteGenetics::FindChromosome( 1274333 ) );
        CHECK_EQUAL( 1, ParasiteGenetics::FindChromosome( ParasiteGenetics::CHROMOSOME_LENGTH[0]+ParasiteGenetics::CHROMOSOME_LENGTH[1] ) );
        CHECK_EQUAL( 2, ParasiteGenetics::FindChromosome( ParasiteGenetics::CHROMOSOME_LENGTH[0]+ParasiteGenetics::CHROMOSOME_LENGTH[1]+1 ) );
        CHECK_EQUAL( 2, ParasiteGenetics::FindChromosome( ParasiteGenetics::CHROMOSOME_LENGTH[0]+ParasiteGenetics::CHROMOSOME_LENGTH[1]+ParasiteGenetics::CHROMOSOME_LENGTH[2] ) );
        CHECK_EQUAL( 3, ParasiteGenetics::FindChromosome( ParasiteGenetics::CHROMOSOME_LENGTH[0]+ParasiteGenetics::CHROMOSOME_LENGTH[1]+ParasiteGenetics::CHROMOSOME_LENGTH[2]+1 ) );
        CHECK_EQUAL( 3, ParasiteGenetics::FindChromosome( ParasiteGenetics::CHROMOSOME_LENGTH[0]+ParasiteGenetics::CHROMOSOME_LENGTH[1]+ParasiteGenetics::CHROMOSOME_LENGTH[2]+ParasiteGenetics::CHROMOSOME_LENGTH[3] ) );
        CHECK_EQUAL( 4, ParasiteGenetics::FindChromosome( ParasiteGenetics::CHROMOSOME_LENGTH[0]+ParasiteGenetics::CHROMOSOME_LENGTH[1]+ParasiteGenetics::CHROMOSOME_LENGTH[2]+ParasiteGenetics::CHROMOSOME_LENGTH[3]+1 ) );

        CHECK_EQUAL( 13, ParasiteGenetics::FindChromosome( ParasiteGenetics::CHROMOSOME_LENGTH[  0 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  1 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  2 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  3 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  4 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  5 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  6 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  7 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  8 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  9 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[ 10 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[ 11 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[ 12 ] +
                                                           1 ) );

        CHECK_EQUAL( 13, ParasiteGenetics::FindChromosome( 20590000 ) );

        CHECK_EQUAL( 13, ParasiteGenetics::FindChromosome( ParasiteGenetics::CHROMOSOME_LENGTH[  0 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  1 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  2 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  3 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  4 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  5 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  6 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  7 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  8 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[  9 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[ 10 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[ 11 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[ 12 ] +
                                                           ParasiteGenetics::CHROMOSOME_LENGTH[ 13 ] ) );
    }

    TEST_FIXTURE( ParasiteFixture, TestConvertCrossoverLocationToIndex )
    {
        PSEUDO_DES rng( 42 );

        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ParasiteGeneticsTest/TestConvertLocationToIndex.json" );

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        CHECK_EQUAL(  0, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 0,      0 ) );
        CHECK_EQUAL(  0, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 0,      1 ) );
        CHECK_EQUAL(  0, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 0,  99999 ) );
        CHECK_EQUAL(  0, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 0, 100000 ) );
        CHECK_EQUAL(  1, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 0, 100001 ) );
        CHECK_EQUAL(  1, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 0, 199999 ) );
        CHECK_EQUAL(  1, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 0, 200000 ) );
        CHECK_EQUAL(  2, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 0, 200001 ) );
        CHECK_EQUAL(  2, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 0, 214333 ) );
        CHECK_EQUAL(  3, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 0, 214334 ) );
        CHECK_EQUAL(  3, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 0, 311500 ) );
        CHECK_EQUAL(  4, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 0, 311501 ) );
        CHECK_EQUAL(  4, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 0, 428667 ) );
        CHECK_EQUAL( -1, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 0, 428668 ) ); // past the last location so nothing left to be copied

        CHECK_EQUAL( -1, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 1,        0 ) ); // 0 is not on the second chromosome
        CHECK_EQUAL(  5, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 1,  ParasiteGenetics::CHROMOSOME_LENGTH[0]+1 ) );
        CHECK_EQUAL(  5, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 1,  958666 ) );
        CHECK_EQUAL(  5, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 1,  958667 ) );
        CHECK_EQUAL(  6, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 1,  958668 ) );
        CHECK_EQUAL(  6, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 1,  999999 ) );
        CHECK_EQUAL(  6, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 1, 1000000 ) );
        CHECK_EQUAL(  7, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 1, 1000001 ) );
        CHECK_EQUAL(  7, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 1, 1116499 ) );
        CHECK_EQUAL(  7, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 1, 1116500 ) );
        CHECK_EQUAL(  8, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 1, 1116501 ) );
        CHECK_EQUAL(  8, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 1, 1116501 ) );
        CHECK_EQUAL(  8, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 1, 1274333 ) );
        CHECK_EQUAL( -1, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 1, 1274334 ) ); // past the last location so nothing left to be copied

        CHECK_EQUAL( 23, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 4, 4999999 ) );
        CHECK_EQUAL( 23, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 4, 5000000 ) );
        CHECK_EQUAL( -1, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 4, 5000001 ) ); // 5000000 was the last location on that chromosome

        CHECK_EQUAL( 73, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 13, 20149999 ) );
        CHECK_EQUAL( 73, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 13, 20150000 ) );
        CHECK_EQUAL( 74, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 13, 20150001 ) );
        CHECK_EQUAL( 74, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 13, 20590000 ) );
        CHECK_EQUAL( 75, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 13, 20590001 ) );
        CHECK_EQUAL( 75, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 13, 20810000 ) );
        CHECK_EQUAL( 76, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 13, 20810001 ) );
        CHECK_EQUAL( 76, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 13, 21470000 ) );
        CHECK_EQUAL( 77, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 13, 21470001 ) );
        CHECK_EQUAL( 77, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 13, 21690000 ) );
        CHECK_EQUAL( 78, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 13, 21690001 ) );
        CHECK_EQUAL( 78, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 13, 22130000 ) );
        CHECK_EQUAL( -1, ParasiteGenetics::CreateInstance()->ConvertCrossoverLocationToIndex( 13, 22130001 ) ); // past the last location so nothing left to be copied
    }

    TEST_FIXTURE( ParasiteFixture, TestFindCrossovers )
    {
        PSEUDO_DES rng( 42 );

        std::list<Crossover> crossovers_list;
        std::vector<Crossover> crossovers;

        ParasiteGenome::FindCrossovers( &rng, 0, crossovers_list );
        std::copy(std::begin(crossovers_list), std::end(crossovers_list), std::back_inserter(crossovers));

        // only expecting the obligate
        CHECK_EQUAL( 1, crossovers.size() );
        CHECK_EQUAL( 495541, crossovers[ 0 ].genome_location  );
        CHECK_EQUAL(      1, crossovers[ 0 ].chromatid_female );
        CHECK_EQUAL(      0, crossovers[ 0 ].chromatid_male   );

        crossovers_list.clear();
        crossovers.clear();

        ParasiteGenome::FindCrossovers( &rng, 1, crossovers_list );

        std::copy(std::begin(crossovers_list), std::end(crossovers_list), std::back_inserter(crossovers));

        CHECK_EQUAL( 1, crossovers.size() );
        CHECK_EQUAL(  693153, crossovers[ 0 ].genome_location  );
        CHECK_EQUAL(       1, crossovers[ 0 ].chromatid_female );
        CHECK_EQUAL(       0, crossovers[ 0 ].chromatid_male   );

        crossovers_list.clear();
        crossovers.clear();

        ParasiteGenome::FindCrossovers( &rng, 2, crossovers_list );

        std::copy(std::begin(crossovers_list), std::end(crossovers_list), std::back_inserter(crossovers));

        CHECK_EQUAL( 1, crossovers.size() );
        CHECK_EQUAL( 2572025, crossovers[ 0 ].genome_location  );
        CHECK_EQUAL(       0, crossovers[ 0 ].chromatid_female );
        CHECK_EQUAL(       1, crossovers[ 0 ].chromatid_male   );

        crossovers_list.clear();
        crossovers.clear();

        ParasiteGenome::FindCrossovers( &rng, 12, crossovers_list );

        std::copy(std::begin(crossovers_list), std::end(crossovers_list), std::back_inserter(crossovers));

        CHECK_EQUAL( 3, crossovers.size() );
        CHECK_EQUAL( 18440022, crossovers[  0 ].genome_location  );
        CHECK_EQUAL(        0, crossovers[  0 ].chromatid_female );
        CHECK_EQUAL(        0, crossovers[  0 ].chromatid_male   );
        CHECK_EQUAL( 18878990, crossovers[  1 ].genome_location  );
        CHECK_EQUAL(        1, crossovers[  1 ].chromatid_female );
        CHECK_EQUAL(        1, crossovers[  1 ].chromatid_male   );
        CHECK_EQUAL( 18984290, crossovers[  2 ].genome_location  );
        CHECK_EQUAL(        0, crossovers[  2 ].chromatid_female );
        CHECK_EQUAL(        0, crossovers[  2 ].chromatid_male   );

        crossovers_list.clear();
        crossovers.clear();

        ParasiteGenome::FindCrossovers( &rng, 13, crossovers_list );

        std::copy(std::begin(crossovers_list), std::end(crossovers_list), std::back_inserter(crossovers));

        CHECK_EQUAL( 3, crossovers.size() );
        CHECK_EQUAL( 19769024, crossovers[  0 ].genome_location  );
        CHECK_EQUAL(        1, crossovers[  0 ].chromatid_female );
        CHECK_EQUAL(        1, crossovers[  0 ].chromatid_male   );
        CHECK_EQUAL( 19961004, crossovers[  1 ].genome_location  );
        CHECK_EQUAL(        0, crossovers[  1 ].chromatid_female );
        CHECK_EQUAL(        1, crossovers[  1 ].chromatid_male   );
        CHECK_EQUAL( 21347739, crossovers[  2 ].genome_location  );
        CHECK_EQUAL(        1, crossovers[  2 ].chromatid_female );
        CHECK_EQUAL(        1, crossovers[  2 ].chromatid_male   );
    }
#endif

    void InitializeSequence( int32_t chromatid, std::vector<int32_t>& rNS )
    {
        int32_t chromatid_val = 1000 * chromatid;

        for( int i_chromosome = 0; i_chromosome < ParasiteGenetics::NUM_CHROMOSOMES; ++i_chromosome )
        {
            int32_t chromosome_val = 10 * (i_chromosome + 1);

            int32_t index_first = ParasiteGenetics::GetInstance()->GetFirstIndexOnChromosome( i_chromosome );
            int32_t index_last  = ParasiteGenetics::GetInstance()->GetLastIndexOnChromosome(  i_chromosome );

            int32_t val = chromatid_val + chromosome_val + 1;
            for( int32_t index = index_first; index <= index_last; ++index )
            {
                rNS[ index ] = val;
                ++val;
            }
        }
    }

#if 1
    TEST_FIXTURE( ParasiteFixture, TestIndependentAssortment )
    {
        RandomFake rng;

        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ParasiteGeneticsTest/TestIndependentAssortment.json" );

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        int32_t num_bp = ParasiteGenetics::GetInstance()->GetNumBasePairs();

        std::vector<int32_t> ns_orig_f0( num_bp, 0 );
        std::vector<int32_t> ns_orig_f1( num_bp, 0 );
        std::vector<int32_t> ns_orig_m0( num_bp, 0 );
        std::vector<int32_t> ns_orig_m1( num_bp, 0 );

        InitializeSequence( 1, ns_orig_f0 );
        InitializeSequence( 2, ns_orig_f1 );
        InitializeSequence( 3, ns_orig_m0 );
        InitializeSequence( 4, ns_orig_m1 );

        std::vector<int32_t> ar_orig_f0( num_bp, 1 );
        std::vector<int32_t> ar_orig_f1( num_bp, 2 );
        std::vector<int32_t> ar_orig_m0( num_bp, 3 );
        std::vector<int32_t> ar_orig_m1( num_bp, 4 );

        ParasiteGenomeInner female_0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner female_1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner male_0(   ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner male_1(   ns_orig_m1, ar_orig_m1 );

        // -------------------------------------------------------------
        // --- Test Assortment 0
        // -------------------------------------------------------------
        rng.SetUL( 0 ); // Set the random number so that uniformZeroToN16( 24 ) = 0

        ParasiteGenome::IndependentAssortment( &rng, 0, &female_0, &female_1, &male_0, &male_1 );

        CHECK_ARRAY_EQUAL( ns_orig_f0, female_0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_orig_f1, female_1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_orig_m0,   male_0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_orig_m1,   male_1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_orig_f0, female_0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_orig_f1, female_1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_orig_m0,   male_0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_orig_m1,   male_1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 1
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 1
        uint32_t ul = 1 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 1;
        rng.SetUL( ul );

        ParasiteGenome::IndependentAssortment( &rng, 0, &female_0, &female_1, &male_0, &male_1 );

        std::vector<int32_t> ns_exp_m0 = {
            4011, 4012, 4013, 4014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        std::vector<int32_t> ns_exp_m1 = {
            3011, 3012, 3013, 3014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };
        std::vector<int32_t> ar_exp_m0 = {
            4, 4, 4, 4,
            3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        std::vector<int32_t> ar_exp_m1 = {
            3, 3, 3, 3,
            4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_orig_f0, female_0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_orig_f1, female_1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0,   male_0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1,   male_1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_orig_f0, female_0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_orig_f1, female_1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0,   male_0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1,   male_1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 2
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 2
        ul = 2 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 2;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_2_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_2_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_2_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_2_m1( ns_orig_m1, ar_orig_m1 );

        // 2nd chromosome
        ParasiteGenome::IndependentAssortment( &rng, 1, &assort_2_f0, &assort_2_f1, &assort_2_m0, &assort_2_m1 );

        std::vector<int32_t> ns_exp_f0 = ns_orig_f0;
        std::vector<int32_t> ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            3021, 3022, 3023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            2021, 2022, 2023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = ns_orig_m1;

        std::vector<int32_t> ar_exp_f0 = ar_orig_f0;
        std::vector<int32_t> ar_exp_f1 = {
            2, 2, 2, 2,
            3, 3, 3,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };

        ar_exp_m0 = {
            3, 3, 3, 3,
            2, 2, 2,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = ar_orig_m1;

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_2_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_2_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_2_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_2_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_2_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_2_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_2_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_2_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 3
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 3
        ul = 3 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 3;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_3_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_3_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_3_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_3_m1( ns_orig_m1, ar_orig_m1 );

        // 3rd chromosome
        ParasiteGenome::IndependentAssortment( &rng, 2, &assort_3_f0, &assort_3_f1, &assort_3_m0, &assort_3_m1 );

        ns_exp_f0 = ns_orig_f0;
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            3031, 3032, 3033, 3034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            4031, 4032, 4033, 4034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            2031, 2032, 2033, 2034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = ar_orig_f0;
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            3, 3, 3, 3,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            4, 4, 4, 4,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            2, 2, 2, 2,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_3_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_3_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_3_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_3_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_3_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_3_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_3_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_3_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 4
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 4
        ul = 4 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 4;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_4_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_4_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_4_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_4_m1( ns_orig_m1, ar_orig_m1 );

        // 4th chromosome
        ParasiteGenome::IndependentAssortment( &rng, 3, &assort_4_f0, &assort_4_f1, &assort_4_m0, &assort_4_m1 );

        ns_exp_f0 = ns_orig_f0;
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            4041, 4042, 4043, 4044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            2041, 2042, 2043, 2044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            3041, 3042, 3043, 3044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = ar_orig_f0;
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            4, 4, 4, 4,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            3, 3, 3, 3,
            2, 2, 2, 2,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            4, 4, 4, 4,
            3, 3, 3, 3,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_4_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_4_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_4_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_4_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_4_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_4_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_4_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_4_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 5
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 5
        ul = 5 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 5;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_5_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_5_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_5_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_5_m1( ns_orig_m1, ar_orig_m1 );

        // 5th chromosome
        ParasiteGenome::IndependentAssortment( &rng, 4, &assort_5_f0, &assort_5_f1, &assort_5_m0, &assort_5_m1 );

        ns_exp_f0 = ns_orig_f0;
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            4051, 4052, 4053, 4054, 4055, 4056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            2051, 2052, 2053, 2054, 2055, 2056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = ar_orig_f0;
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
            4, 4, 4, 4, 4, 4,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4,
            2, 2, 2, 2, 2, 2,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_5_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_5_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_5_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_5_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_5_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_5_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_5_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_5_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 6
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 6
        ul = 6 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 6;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_6_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_6_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_6_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_6_m1( ns_orig_m1, ar_orig_m1 );

        // 6th chromosome
        ParasiteGenome::IndependentAssortment( &rng, 5, &assort_6_f0, &assort_6_f1, &assort_6_m0, &assort_6_m1 );

        ns_exp_f0 = {
            1011, 1012, 1013, 1014,
            1021, 1022, 1023,
            1031, 1032, 1033, 1034,
            1041, 1042, 1043, 1044,
            1051, 1052, 1053, 1054, 1055, 1056,
            2061, 2062, 2063, 2064, 2065, 2066,
            1071, 1072, 1073, 1074, 1075, 1076,
            1081, 1082, 1083, 1084, 1085, 1086,
            1091, 1092, 1093, 1094, 1095, 1096,
            1101, 1102, 1103, 1104, 1105, 1106,
            1111, 1112, 1113, 1114, 1115, 1116,
            1121, 1122, 1123, 1124, 1125, 1126,
            1131, 1132, 1133, 1134, 1135, 1136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            1061, 1062, 1063, 1064, 1065, 1066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            1, 1, 1, 1,
            1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_6_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_6_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_6_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_6_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_6_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_6_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_6_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_6_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 7
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 7
        ul = 7 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 7;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_7_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_7_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_7_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_7_m1( ns_orig_m1, ar_orig_m1 );

        // 7th chromosome
        ParasiteGenome::IndependentAssortment( &rng, 6, &assort_7_f0, &assort_7_f1, &assort_7_m0, &assort_7_m1 );

        ns_exp_f0 = {
            1011, 1012, 1013, 1014,
            1021, 1022, 1023,
            1031, 1032, 1033, 1034,
            1041, 1042, 1043, 1044,
            1051, 1052, 1053, 1054, 1055, 1056,
            1061, 1062, 1063, 1064, 1065, 1066,
            2071, 2072, 2073, 2074, 2075, 2076,
            1081, 1082, 1083, 1084, 1085, 1086,
            1091, 1092, 1093, 1094, 1095, 1096,
            1101, 1102, 1103, 1104, 1105, 1106,
            1111, 1112, 1113, 1114, 1115, 1116,
            1121, 1122, 1123, 1124, 1125, 1126,
            1131, 1132, 1133, 1134, 1135, 1136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            1071, 1072, 1073, 1074, 1075, 1076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            4071, 4072, 4073, 4074, 4075, 4076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            3071, 3072, 3073, 3074, 3075, 3076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            1, 1, 1, 1,
            1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            4, 4, 4, 4, 4, 4,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            3, 3, 3, 3, 3, 3,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_7_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_7_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_7_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_7_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_7_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_7_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_7_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_7_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 8
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 8
        ul = 8 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 8;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_8_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_8_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_8_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_8_m1( ns_orig_m1, ar_orig_m1 );

        // 8th chromosome
        ParasiteGenome::IndependentAssortment( &rng, 7, &assort_8_f0, &assort_8_f1, &assort_8_m0, &assort_8_m1 );

        ns_exp_f0 = {
            1011, 1012, 1013, 1014,
            1021, 1022, 1023,
            1031, 1032, 1033, 1034,
            1041, 1042, 1043, 1044,
            1051, 1052, 1053, 1054, 1055, 1056,
            1061, 1062, 1063, 1064, 1065, 1066,
            1071, 1072, 1073, 1074, 1075, 1076,
            2081, 2082, 2083, 2084, 2085, 2086,
            1091, 1092, 1093, 1094, 1095, 1096,
            1101, 1102, 1103, 1104, 1105, 1106,
            1111, 1112, 1113, 1114, 1115, 1116,
            1121, 1122, 1123, 1124, 1125, 1126,
            1131, 1132, 1133, 1134, 1135, 1136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            3081, 3082, 3083, 3084, 3085, 3086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            1081, 1082, 1083, 1084, 1085, 1086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            1, 1, 1, 1,
            1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            3, 3, 3, 3, 3, 3,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            1, 1, 1, 1, 1, 1,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_8_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_8_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_8_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_8_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_8_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_8_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_8_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_8_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 9
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 9
        ul = 9 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 9;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_9_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_9_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_9_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_9_m1( ns_orig_m1, ar_orig_m1 );

        // 9th chromosome
        ParasiteGenome::IndependentAssortment( &rng, 8, &assort_9_f0, &assort_9_f1, &assort_9_m0, &assort_9_m1 );

        ns_exp_f0 = {
            1011, 1012, 1013, 1014,
            1021, 1022, 1023,
            1031, 1032, 1033, 1034,
            1041, 1042, 1043, 1044,
            1051, 1052, 1053, 1054, 1055, 1056,
            1061, 1062, 1063, 1064, 1065, 1066,
            1071, 1072, 1073, 1074, 1075, 1076,
            1081, 1082, 1083, 1084, 1085, 1086,
            2091, 2092, 2093, 2094, 2095, 2096,
            1101, 1102, 1103, 1104, 1105, 1106,
            1111, 1112, 1113, 1114, 1115, 1116,
            1121, 1122, 1123, 1124, 1125, 1126,
            1131, 1132, 1133, 1134, 1135, 1136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            3091, 3092, 3093, 3094, 3095, 3096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            4091, 4092, 4093, 4094, 4095, 4096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            1091, 1092, 1093, 1094, 1095, 1096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            1, 1, 1, 1,
            1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            3, 3, 3, 3, 3, 3,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            4, 4, 4, 4, 4, 4,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            1, 1, 1, 1, 1, 1,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_9_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_9_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_9_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_9_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_9_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_9_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_9_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_9_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 10
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 10
        ul = 10 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 10;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_10_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_10_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_10_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_10_m1( ns_orig_m1, ar_orig_m1 );

        // 10th chromosome
        ParasiteGenome::IndependentAssortment( &rng, 9, &assort_10_f0, &assort_10_f1, &assort_10_m0, &assort_10_m1 );

        ns_exp_f0 = {
            1011, 1012, 1013, 1014,
            1021, 1022, 1023,
            1031, 1032, 1033, 1034,
            1041, 1042, 1043, 1044,
            1051, 1052, 1053, 1054, 1055, 1056,
            1061, 1062, 1063, 1064, 1065, 1066,
            1071, 1072, 1073, 1074, 1075, 1076,
            1081, 1082, 1083, 1084, 1085, 1086,
            1091, 1092, 1093, 1094, 1095, 1096,
            2101, 2102, 2103, 2104, 2105, 2106,
            1111, 1112, 1113, 1114, 1115, 1116,
            1121, 1122, 1123, 1124, 1125, 1126,
            1131, 1132, 1133, 1134, 1135, 1136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            4101, 4102, 4103, 4104, 4105, 4106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            1101, 1102, 1103, 1104, 1105, 1106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            3101, 3102, 3103, 3104, 3105, 3106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            1, 1, 1, 1,
            1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            4, 4, 4, 4, 4, 4,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            1, 1, 1, 1, 1, 1,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            3, 3, 3, 3, 3, 3,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_10_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_10_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_10_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_10_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_10_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_10_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_10_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_10_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 11
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 11
        ul = 11 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 11;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_11_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_11_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_11_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_11_m1( ns_orig_m1, ar_orig_m1 );

        // 11th chromosome
        ParasiteGenome::IndependentAssortment( &rng, 10, &assort_11_f0, &assort_11_f1, &assort_11_m0, &assort_11_m1 );

        ns_exp_f0 = {
            1011, 1012, 1013, 1014,
            1021, 1022, 1023,
            1031, 1032, 1033, 1034,
            1041, 1042, 1043, 1044,
            1051, 1052, 1053, 1054, 1055, 1056,
            1061, 1062, 1063, 1064, 1065, 1066,
            1071, 1072, 1073, 1074, 1075, 1076,
            1081, 1082, 1083, 1084, 1085, 1086,
            1091, 1092, 1093, 1094, 1095, 1096,
            1101, 1102, 1103, 1104, 1105, 1106,
            2111, 2112, 2113, 2114, 2115, 2116,
            1121, 1122, 1123, 1124, 1125, 1126,
            1131, 1132, 1133, 1134, 1135, 1136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            4111, 4112, 4113, 4114, 4115, 4116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            1111, 1112, 1113, 1114, 1115, 1116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            1, 1, 1, 1,
            1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            4, 4, 4, 4, 4, 4,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            1, 1, 1, 1, 1, 1,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_11_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_11_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_11_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_11_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_11_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_11_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_11_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_11_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 12
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 12
        ul = 12 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 12;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_12_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_12_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_12_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_12_m1( ns_orig_m1, ar_orig_m1 );

        // 12th chromosome
        ParasiteGenome::IndependentAssortment( &rng, 11, &assort_12_f0, &assort_12_f1, &assort_12_m0, &assort_12_m1 );

        ns_exp_f0 = {
            1011, 1012, 1013, 1014,
            1021, 1022, 1023,
            1031, 1032, 1033, 1034,
            1041, 1042, 1043, 1044,
            1051, 1052, 1053, 1054, 1055, 1056,
            1061, 1062, 1063, 1064, 1065, 1066,
            1071, 1072, 1073, 1074, 1075, 1076,
            1081, 1082, 1083, 1084, 1085, 1086,
            1091, 1092, 1093, 1094, 1095, 1096,
            1101, 1102, 1103, 1104, 1105, 1106,
            1111, 1112, 1113, 1114, 1115, 1116,
            3121, 3122, 3123, 3124, 3125, 3126,
            1131, 1132, 1133, 1134, 1135, 1136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            1121, 1122, 1123, 1124, 1125, 1126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            2121, 2122, 2123, 2124, 2125, 2126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            1, 1, 1, 1,
            1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            3, 3, 3, 3, 3, 3,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            2, 2, 2, 2, 2, 2,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_12_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_12_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_12_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_12_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_12_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_12_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_12_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_12_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 13
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 13
        ul = 13 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 13;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_13_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_13_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_13_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_13_m1( ns_orig_m1, ar_orig_m1 );

        // 13th chromosome
        ParasiteGenome::IndependentAssortment( &rng, 12, &assort_13_f0, &assort_13_f1, &assort_13_m0, &assort_13_m1 );

        ns_exp_f0 = {
            1011, 1012, 1013, 1014,
            1021, 1022, 1023,
            1031, 1032, 1033, 1034,
            1041, 1042, 1043, 1044,
            1051, 1052, 1053, 1054, 1055, 1056,
            1061, 1062, 1063, 1064, 1065, 1066,
            1071, 1072, 1073, 1074, 1075, 1076,
            1081, 1082, 1083, 1084, 1085, 1086,
            1091, 1092, 1093, 1094, 1095, 1096,
            1101, 1102, 1103, 1104, 1105, 1106,
            1111, 1112, 1113, 1114, 1115, 1116,
            1121, 1122, 1123, 1124, 1125, 1126,
            3131, 3132, 3133, 3134, 3135, 3136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            1131, 1132, 1133, 1134, 1135, 1136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            4131, 4132, 4133, 4134, 4135, 4136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            2131, 2132, 2133, 2134, 2135, 2136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            1, 1, 1, 1,
            1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            3, 3, 3, 3, 3, 3,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            4, 4, 4, 4, 4, 4,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            2, 2, 2, 2, 2, 2,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_13_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_13_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_13_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_13_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_13_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_13_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_13_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_13_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 14
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 14
        ul = 14 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 14;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_14_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_14_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_14_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_14_m1( ns_orig_m1, ar_orig_m1 );

        // 14th chromosome
        ParasiteGenome::IndependentAssortment( &rng, 13, &assort_14_f0, &assort_14_f1, &assort_14_m0, &assort_14_m1 );

        ns_exp_f0 = {
            1011, 1012, 1013, 1014,
            1021, 1022, 1023,
            1031, 1032, 1033, 1034,
            1041, 1042, 1043, 1044,
            1051, 1052, 1053, 1054, 1055, 1056,
            1061, 1062, 1063, 1064, 1065, 1066,
            1071, 1072, 1073, 1074, 1075, 1076,
            1081, 1082, 1083, 1084, 1085, 1086,
            1091, 1092, 1093, 1094, 1095, 1096,
            1101, 1102, 1103, 1104, 1105, 1106,
            1111, 1112, 1113, 1114, 1115, 1116,
            1121, 1122, 1123, 1124, 1125, 1126,
            1131, 1132, 1133, 1134, 1135, 1136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            1, 1, 1, 1,
            1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_14_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_14_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_14_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_14_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_14_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_14_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_14_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_14_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 15
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 15
        ul = 15 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 15;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_15_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_15_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_15_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_15_m1( ns_orig_m1, ar_orig_m1 );

        // 1st chromosome
        ParasiteGenome::IndependentAssortment( &rng, 0, &assort_15_f0, &assort_15_f1, &assort_15_m0, &assort_15_m1 );

        ns_exp_f0 = {
            3011, 3012, 3013, 3014,
            1021, 1022, 1023,
            1031, 1032, 1033, 1034,
            1041, 1042, 1043, 1044,
            1051, 1052, 1053, 1054, 1055, 1056,
            1061, 1062, 1063, 1064, 1065, 1066,
            1071, 1072, 1073, 1074, 1075, 1076,
            1081, 1082, 1083, 1084, 1085, 1086,
            1091, 1092, 1093, 1094, 1095, 1096,
            1101, 1102, 1103, 1104, 1105, 1106,
            1111, 1112, 1113, 1114, 1115, 1116,
            1121, 1122, 1123, 1124, 1125, 1126,
            1131, 1132, 1133, 1134, 1135, 1136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            4011, 4012, 4013, 4014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            1011, 1012, 1013, 1014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            3, 3, 3, 3,
            1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            4, 4, 4, 4,
            3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            1, 1, 1, 1,
            4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_15_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_15_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_15_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_15_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_15_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_15_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_15_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_15_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 16
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 16
        ul = 16 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 16;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_16_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_16_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_16_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_16_m1( ns_orig_m1, ar_orig_m1 );

        // 2nd chromosome
        ParasiteGenome::IndependentAssortment( &rng, 1, &assort_16_f0, &assort_16_f1, &assort_16_m0, &assort_16_m1 );

        ns_exp_f0 = {
            1011, 1012, 1013, 1014,
            3021, 3022, 3023,
            1031, 1032, 1033, 1034,
            1041, 1042, 1043, 1044,
            1051, 1052, 1053, 1054, 1055, 1056,
            1061, 1062, 1063, 1064, 1065, 1066,
            1071, 1072, 1073, 1074, 1075, 1076,
            1081, 1082, 1083, 1084, 1085, 1086,
            1091, 1092, 1093, 1094, 1095, 1096,
            1101, 1102, 1103, 1104, 1105, 1106,
            1111, 1112, 1113, 1114, 1115, 1116,
            1121, 1122, 1123, 1124, 1125, 1126,
            1131, 1132, 1133, 1134, 1135, 1136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            4021, 4022, 4023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            1021, 1022, 1023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            2021, 2022, 2023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            1, 1, 1, 1,
            3, 3, 3,
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            4, 4, 4,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            1, 1, 1,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            2, 2, 2,
            4, 4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_16_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_16_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_16_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_16_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_16_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_16_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_16_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_16_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 17
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 17
        ul = 17 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 17;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_17_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_17_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_17_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_17_m1( ns_orig_m1, ar_orig_m1 );

        // 3rd chromosome
        ParasiteGenome::IndependentAssortment( &rng, 2, &assort_17_f0, &assort_17_f1, &assort_17_m0, &assort_17_m1 );

        ns_exp_f0 = {
            1011, 1012, 1013, 1014,
            1021, 1022, 1023,
            3031, 3032, 3033, 3034,
            1041, 1042, 1043, 1044,
            1051, 1052, 1053, 1054, 1055, 1056,
            1061, 1062, 1063, 1064, 1065, 1066,
            1071, 1072, 1073, 1074, 1075, 1076,
            1081, 1082, 1083, 1084, 1085, 1086,
            1091, 1092, 1093, 1094, 1095, 1096,
            1101, 1102, 1103, 1104, 1105, 1106,
            1111, 1112, 1113, 1114, 1115, 1116,
            1121, 1122, 1123, 1124, 1125, 1126,
            1131, 1132, 1133, 1134, 1135, 1136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            4031, 4032, 4033, 4034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            2031, 2032, 2033, 2034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            1031, 1032, 1033, 1034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            1, 1, 1, 1,
            1, 1, 1, 
            3, 3, 3, 3,
            1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            4, 4, 4, 4,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            2, 2, 2, 2,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            1, 1, 1, 1,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_17_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_17_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_17_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_17_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_17_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_17_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_17_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_17_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 18
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 18
        ul = 18 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 18;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_18_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_18_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_18_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_18_m1( ns_orig_m1, ar_orig_m1 );

        // 4th chromosome
        ParasiteGenome::IndependentAssortment( &rng, 3, &assort_18_f0, &assort_18_f1, &assort_18_m0, &assort_18_m1 );

        ns_exp_f0 = {
            1011, 1012, 1013, 1014,
            1021, 1022, 1023,
            1031, 1032, 1033, 1034,
            4041, 4042, 4043, 4044,
            1051, 1052, 1053, 1054, 1055, 1056,
            1061, 1062, 1063, 1064, 1065, 1066,
            1071, 1072, 1073, 1074, 1075, 1076,
            1081, 1082, 1083, 1084, 1085, 1086,
            1091, 1092, 1093, 1094, 1095, 1096,
            1101, 1102, 1103, 1104, 1105, 1106,
            1111, 1112, 1113, 1114, 1115, 1116,
            1121, 1122, 1123, 1124, 1125, 1126,
            1131, 1132, 1133, 1134, 1135, 1136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            1041, 1042, 1043, 1044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            2041, 2042, 2043, 2044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            3041, 3042, 3043, 3044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            1, 1, 1, 1,
            1, 1, 1, 
            1, 1, 1, 1,
            4, 4, 4, 4,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            1, 1, 1, 1,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            3, 3, 3, 3,
            2, 2, 2, 2,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            4, 4, 4, 4,
            3, 3, 3, 3,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_18_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_18_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_18_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_18_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_18_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_18_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_18_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_18_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 19
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 19
        ul = 19 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 19;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_19_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_19_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_19_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_19_m1( ns_orig_m1, ar_orig_m1 );

        // 5th chromosome
        ParasiteGenome::IndependentAssortment( &rng, 4, &assort_19_f0, &assort_19_f1, &assort_19_m0, &assort_19_m1 );

        ns_exp_f0 = {
            1011, 1012, 1013, 1014,
            1021, 1022, 1023,
            1031, 1032, 1033, 1034,
            1041, 1042, 1043, 1044,
            4051, 4052, 4053, 4054, 4055, 4056,
            1061, 1062, 1063, 1064, 1065, 1066,
            1071, 1072, 1073, 1074, 1075, 1076,
            1081, 1082, 1083, 1084, 1085, 1086,
            1091, 1092, 1093, 1094, 1095, 1096,
            1101, 1102, 1103, 1104, 1105, 1106,
            1111, 1112, 1113, 1114, 1115, 1116,
            1121, 1122, 1123, 1124, 1125, 1126,
            1131, 1132, 1133, 1134, 1135, 1136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            1051, 1052, 1053, 1054, 1055, 1056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            2051, 2052, 2053, 2054, 2055, 2056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            1, 1, 1, 1,
            1, 1, 1, 
            1, 1, 1, 1,
            1, 1, 1, 1,
            4, 4, 4, 4, 4, 4,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
            1, 1, 1, 1, 1, 1,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4,
            2, 2, 2, 2, 2, 2,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_19_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_19_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_19_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_19_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_19_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_19_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_19_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_19_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 20
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 20
        ul = 20 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 20;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_20_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_20_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_20_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_20_m1( ns_orig_m1, ar_orig_m1 );

        // 6th chromosome
        ParasiteGenome::IndependentAssortment( &rng, 5, &assort_20_f0, &assort_20_f1, &assort_20_m0, &assort_20_m1 );

        ns_exp_f0 = {
            1011, 1012, 1013, 1014,
            1021, 1022, 1023,
            1031, 1032, 1033, 1034,
            1041, 1042, 1043, 1044,
            1051, 1052, 1053, 1054, 1055, 1056,
            4061, 4062, 4063, 4064, 4065, 4066,
            1071, 1072, 1073, 1074, 1075, 1076,
            1081, 1082, 1083, 1084, 1085, 1086,
            1091, 1092, 1093, 1094, 1095, 1096,
            1101, 1102, 1103, 1104, 1105, 1106,
            1111, 1112, 1113, 1114, 1115, 1116,
            1121, 1122, 1123, 1124, 1125, 1126,
            1131, 1132, 1133, 1134, 1135, 1136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            1061, 1062, 1063, 1064, 1065, 1066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            3061, 3062, 3063, 3064, 3065, 3066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            1, 1, 1, 1,
            1, 1, 1, 
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            4, 4, 4, 4, 4, 4,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            1, 1, 1, 1, 1, 1,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            3, 3, 3, 3, 3, 3,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_20_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_20_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_20_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_20_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_20_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_20_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_20_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_20_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 21
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 21
        ul = 21 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 21;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_21_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_21_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_21_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_21_m1( ns_orig_m1, ar_orig_m1 );

        // 7th chromosome
        ParasiteGenome::IndependentAssortment( &rng, 6, &assort_21_f0, &assort_21_f1, &assort_21_m0, &assort_21_m1 );

        ns_exp_f0 = {
            1011, 1012, 1013, 1014,
            1021, 1022, 1023,
            1031, 1032, 1033, 1034,
            1041, 1042, 1043, 1044,
            1051, 1052, 1053, 1054, 1055, 1056,
            1061, 1062, 1063, 1064, 1065, 1066,
            4071, 4072, 4073, 4074, 4075, 4076,
            1081, 1082, 1083, 1084, 1085, 1086,
            1091, 1092, 1093, 1094, 1095, 1096,
            1101, 1102, 1103, 1104, 1105, 1106,
            1111, 1112, 1113, 1114, 1115, 1116,
            1121, 1122, 1123, 1124, 1125, 1126,
            1131, 1132, 1133, 1134, 1135, 1136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            1071, 1072, 1073, 1074, 1075, 1076,
            4081, 4082, 4083, 4084, 4085, 4086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            1, 1, 1, 1,
            1, 1, 1, 
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            4, 4, 4, 4, 4, 4,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            1, 1, 1, 1, 1, 1,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_21_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_21_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_21_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_21_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_21_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_21_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_21_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_21_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 22
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 22
        ul = 22 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 22;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_22_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_22_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_22_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_22_m1( ns_orig_m1, ar_orig_m1 );

        // 8th chromosome
        ParasiteGenome::IndependentAssortment( &rng, 7, &assort_22_f0, &assort_22_f1, &assort_22_m0, &assort_22_m1 );

        ns_exp_f0 = {
            1011, 1012, 1013, 1014,
            1021, 1022, 1023,
            1031, 1032, 1033, 1034,
            1041, 1042, 1043, 1044,
            1051, 1052, 1053, 1054, 1055, 1056,
            1061, 1062, 1063, 1064, 1065, 1066,
            1071, 1072, 1073, 1074, 1075, 1076,
            4081, 4082, 4083, 4084, 4085, 4086,
            1091, 1092, 1093, 1094, 1095, 1096,
            1101, 1102, 1103, 1104, 1105, 1106,
            1111, 1112, 1113, 1114, 1115, 1116,
            1121, 1122, 1123, 1124, 1125, 1126,
            1131, 1132, 1133, 1134, 1135, 1136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            3081, 3082, 3083, 3084, 3085, 3086,
            2091, 2092, 2093, 2094, 2095, 2096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            1081, 1082, 1083, 1084, 1085, 1086,
            3091, 3092, 3093, 3094, 3095, 3096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            2081, 2082, 2083, 2084, 2085, 2086,
            4091, 4092, 4093, 4094, 4095, 4096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            1, 1, 1, 1,
            1, 1, 1, 
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            4, 4, 4, 4, 4, 4,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            3, 3, 3, 3, 3, 3,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            1, 1, 1, 1, 1, 1,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            2, 2, 2, 2, 2, 2,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_22_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_22_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_22_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_22_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_22_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_22_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_22_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_22_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // -------------------------------------------------------------
        // --- Test Assortment 23
        // -------------------------------------------------------------
        // Set the random number so that uniformZeroToN16( 24 ) = 23
        ul = 23 * uint32_t( uint64_t( UINT_MAX ) / uint64_t( 24 ) ) + 23;
        rng.SetUL( ul );

        ParasiteGenomeInner assort_23_f0( ns_orig_f0, ar_orig_f0 );
        ParasiteGenomeInner assort_23_f1( ns_orig_f1, ar_orig_f1 );
        ParasiteGenomeInner assort_23_m0( ns_orig_m0, ar_orig_m0 );
        ParasiteGenomeInner assort_23_m1( ns_orig_m1, ar_orig_m1 );

        // 9th chromosome
        ParasiteGenome::IndependentAssortment( &rng, 8, &assort_23_f0, &assort_23_f1, &assort_23_m0, &assort_23_m1 );

        ns_exp_f0 = {
            1011, 1012, 1013, 1014,
            1021, 1022, 1023,
            1031, 1032, 1033, 1034,
            1041, 1042, 1043, 1044,
            1051, 1052, 1053, 1054, 1055, 1056,
            1061, 1062, 1063, 1064, 1065, 1066,
            1071, 1072, 1073, 1074, 1075, 1076,
            1081, 1082, 1083, 1084, 1085, 1086,
            4091, 4092, 4093, 4094, 4095, 4096,
            1101, 1102, 1103, 1104, 1105, 1106,
            1111, 1112, 1113, 1114, 1115, 1116,
            1121, 1122, 1123, 1124, 1125, 1126,
            1131, 1132, 1133, 1134, 1135, 1136,
            1141, 1142, 1143, 1144, 1145, 1146
        };
        ns_exp_f1 = {
            2011, 2012, 2013, 2014,
            2021, 2022, 2023,
            2031, 2032, 2033, 2034,
            2041, 2042, 2043, 2044,
            2051, 2052, 2053, 2054, 2055, 2056,
            2061, 2062, 2063, 2064, 2065, 2066,
            2071, 2072, 2073, 2074, 2075, 2076,
            2081, 2082, 2083, 2084, 2085, 2086,
            3091, 3092, 3093, 3094, 3095, 3096,
            2101, 2102, 2103, 2104, 2105, 2106,
            2111, 2112, 2113, 2114, 2115, 2116,
            2121, 2122, 2123, 2124, 2125, 2126,
            2131, 2132, 2133, 2134, 2135, 2136,
            2141, 2142, 2143, 2144, 2145, 2146
        };
        ns_exp_m0 = {
            3011, 3012, 3013, 3014,
            3021, 3022, 3023,
            3031, 3032, 3033, 3034,
            3041, 3042, 3043, 3044,
            3051, 3052, 3053, 3054, 3055, 3056,
            3061, 3062, 3063, 3064, 3065, 3066,
            3071, 3072, 3073, 3074, 3075, 3076,
            3081, 3082, 3083, 3084, 3085, 3086,
            2091, 2092, 2093, 2094, 2095, 2096,
            3101, 3102, 3103, 3104, 3105, 3106,
            3111, 3112, 3113, 3114, 3115, 3116,
            3121, 3122, 3123, 3124, 3125, 3126,
            3131, 3132, 3133, 3134, 3135, 3136,
            3141, 3142, 3143, 3144, 3145, 3146
        };
        ns_exp_m1 = {
            4011, 4012, 4013, 4014,
            4021, 4022, 4023,
            4031, 4032, 4033, 4034,
            4041, 4042, 4043, 4044,
            4051, 4052, 4053, 4054, 4055, 4056,
            4061, 4062, 4063, 4064, 4065, 4066,
            4071, 4072, 4073, 4074, 4075, 4076,
            4081, 4082, 4083, 4084, 4085, 4086,
            1091, 1092, 1093, 1094, 1095, 1096,
            4101, 4102, 4103, 4104, 4105, 4106,
            4111, 4112, 4113, 4114, 4115, 4116,
            4121, 4122, 4123, 4124, 4125, 4126,
            4131, 4132, 4133, 4134, 4135, 4136,
            4141, 4142, 4143, 4144, 4145, 4146
        };

        ar_exp_f0 = {
            1, 1, 1, 1,
            1, 1, 1, 
            1, 1, 1, 1,
            1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            4, 4, 4, 4, 4, 4,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1
        };
        ar_exp_f1 = {
            2, 2, 2, 2,
            2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            3, 3, 3, 3, 3, 3,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2
        };
        ar_exp_m0 = {
            3, 3, 3, 3,
            3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            2, 2, 2, 2, 2, 2,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3,
            3, 3, 3, 3, 3, 3
        };
        ar_exp_m1 = {
            4, 4, 4, 4,
            4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            1, 1, 1, 1, 1, 1,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4,
            4, 4, 4, 4, 4, 4
        };

        CHECK_ARRAY_EQUAL( ns_exp_f0, assort_23_f0.GetNucleotideSequence(), ns_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_f1, assort_23_f1.GetNucleotideSequence(), ns_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m0, assort_23_m0.GetNucleotideSequence(), ns_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ns_exp_m1, assort_23_m1.GetNucleotideSequence(), ns_orig_m1.size() );

        CHECK_ARRAY_EQUAL( ar_exp_f0, assort_23_f0.GetAlleleRoots(), ar_orig_f0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_f1, assort_23_f1.GetAlleleRoots(), ar_orig_f1.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m0, assort_23_m0.GetAlleleRoots(), ar_orig_m0.size() );
        CHECK_ARRAY_EQUAL( ar_exp_m1, assort_23_m1.GetAlleleRoots(), ar_orig_m1.size() );

        // ----------------------------------------------------------------------------
        // --- This is needed because there are local ParasiteGenomeInner objects that
        // --- are being put into the Genome Map.  Hence, we just want to clear it
        // --- so we don't try to re-delete them.
        // ----------------------------------------------------------------------------
        ParasiteGenetics::CreateInstance()->ClearGenomeMap();
    }
#endif

#if 1
    TEST_FIXTURE( ParasiteFixture, TestRecombination )
    {
        PSEUDO_DES rng( 42 );

        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ParasiteGeneticsTest/TestRecombination.json" );
        SusceptibilityMalariaConfig::falciparumPfEMP1Vars = 300;

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        // ------------------------------------------------------------------
        // --- Create a base genomes for testing
        // ------------------------------------------------------------------
        std::string barcode_1 = "AAAAAAAAAAAAAAAAAAAAAAAA";
        std::string drug = "";
        std::string hrp = "";
        int32_t msp_1 = 1;
        std::vector<int32_t> pfemp1_values_1 = {
            100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
            110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
            120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
            130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
            140, 141, 142, 143, 144, 145, 146, 147, 148, 149
        };
        ParasiteGenome pg_tmp_1 = ParasiteGenetics::GetInstance()->CreateGenomeFromSequence( &rng, barcode_1, drug, hrp, msp_1, pfemp1_values_1 );
        ParasiteGenome pg_1 = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_1, 1 );

        CHECK_EQUAL(          2, pg_1.GetID() );
        CHECK_EQUAL(      msp_1, pg_1.GetMSP() );
        CHECK_EQUAL(  barcode_1, pg_1.GetBarcode() );

        std::vector<int32_t> pfemp1_values_actual = pg_1.GetPfEMP1EpitopesMajor();
        CHECK_EQUAL( 50, pfemp1_values_actual.size() ); //CLONAL_PfEMP1_VARIANTS
        CHECK_ARRAY_EQUAL( pfemp1_values_1, pfemp1_values_actual, pfemp1_values_actual.size() );

        std::string barcode_2 = "TTTTTTTTTTTTTTTTTTTTTTTT";
        int32_t msp_2 = 2;
        std::vector<int32_t> pfemp1_values_2 = {
            200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
            210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
            220, 221, 222, 223, 224, 225, 226, 227, 228, 229,
            230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
            240, 241, 242, 243, 244, 245, 246, 247, 248, 249
        };
        ParasiteGenome pg_tmp_2 = ParasiteGenetics::GetInstance()->CreateGenomeFromSequence( &rng, barcode_2, drug, hrp, msp_2, pfemp1_values_2 );
        ParasiteGenome pg_2 = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_2, 2 );

        CHECK_EQUAL(          4, pg_2.GetID() );
        CHECK_EQUAL(      msp_2, pg_2.GetMSP() );
        CHECK_EQUAL(  barcode_2, pg_2.GetBarcode() );

        pfemp1_values_actual = pg_2.GetPfEMP1EpitopesMajor();
        CHECK_EQUAL( 50, pfemp1_values_actual.size() ); //CLONAL_PfEMP1_VARIANTS
        CHECK_ARRAY_EQUAL( pfemp1_values_2, pfemp1_values_actual, pfemp1_values_actual.size() );

        std::vector<ParasiteGenome> new_genomes;

        // --------------------------------------------------------------------------------
        // --- Test that recombination on two identical genomes just gives back that genome
        // --------------------------------------------------------------------------------
        ParasiteGenome::Recombination( &rng, pg_1, pg_1, new_genomes );
        CHECK_EQUAL( 1, new_genomes.size() );
        CHECK_EQUAL( pg_1.GetID(), new_genomes[ 0 ].GetID() );

        // ---------------------------------------------------------------------
        // --- Test that recombination does not move data into the wrong places
        // ---------------------------------------------------------------------

        for( int i_sample = 0; i_sample < 1000; ++i_sample )
        {
            new_genomes.clear();

            ParasiteGenome::Recombination( &rng, pg_1, pg_2, new_genomes );
            CHECK_EQUAL( 4, new_genomes.size() );

            // verify that each PfEMP1 location is maintained in the correct location
            // (i.e. it didn't move somehow) and that there are 2 from each genome
            for( int i = 0; i < pg_1.GetPfEMP1EpitopesMajor().size(); ++i )
            {
                int num_100 = 0;
                int num_200 = 0;

                for( auto& genome : new_genomes )
                {
                    int32_t val = genome.GetPfEMP1EpitopesMajor()[i];
                    if( val >= 200 )
                    {
                        num_200 += 1;
                        val -= 200;
                    }
                    else
                    {
                        num_100 += 1;
                        val -= 100;
                    }
                    CHECK_EQUAL( i, val );
                }
                CHECK_EQUAL( 2, num_100 );
                CHECK_EQUAL( 2, num_200 );
            }

            // verify that for MSP there are 2-1's and 2-2's
            int num_1 = 0;
            int num_2 = 0;
            for( auto& genome : new_genomes )
            {
                num_1 += (genome.GetMSP() == 1) ? 1 : 0;
                num_2 += (genome.GetMSP() == 2) ? 1 : 0;
            }
            CHECK_EQUAL( 2, num_1 );
            CHECK_EQUAL( 2, num_2 );

            // verify that at each barcode location there are 2-T's and 2-A's
            for( int i = 0; i < barcode_1.length(); ++i )
            {
                int num_A = 0;
                int num_T = 0;

                for( auto& genome : new_genomes )
                {
                    num_A += (genome.GetBarcode()[i] == 'A') ? 1 : 0;
                    num_T += (genome.GetBarcode()[i] == 'T') ? 1 : 0;
                }
                CHECK_EQUAL( 2, num_A );
                CHECK_EQUAL( 2, num_T );
            }

            // verify that at each allele root location there are 2-1's and 2-2's
            for( int i = 0; i < pg_1.GetAlleleRoots().size(); ++i )
            {
                num_1 = 0;
                num_2 = 0;

                for( auto& genome : new_genomes )
                {
                    num_1 += (genome.GetAlleleRoots()[i] == 1) ? 1 : 0;
                    num_2 += (genome.GetAlleleRoots()[i] == 2) ? 1 : 0;
                }
                CHECK_EQUAL( 2, num_1 );
                CHECK_EQUAL( 2, num_2 );
            }
        }
    }
#endif

#if 1
    TEST_FIXTURE( ParasiteFixture, TestRecombinationLargeBarcode )
    {
        // --------------------------------------------------------
        // --- Handy code for generating "Barcode_Genome_Locations"
        // --------------------------------------------------------
        //int32_t num_snps_per_chromosome = 1000;
        //for( int i_chromosome = 0; i_chromosome < ParasiteGenetics::NUM_CHROMOSOMES; ++i_chromosome )
        //{
        //    int32_t first_snp = (i_chromosome == 0) ? 1 : ParasiteGenetics::CHROMOSOME_ENDS[ i_chromosome - 1 ] + 1;
        //    int32_t amount_between_snps = ParasiteGenetics::CHROMOSOME_LENGTH[ i_chromosome ] / num_snps_per_chromosome ;
        //    for( int snp = first_snp; snp < ParasiteGenetics::CHROMOSOME_ENDS[ i_chromosome ]; snp += amount_between_snps )
        //    {
        //        printf("%d,",snp);
        //    }
        //    printf("\n");
        //}
        // --------------------------------------------------------

        PSEUDO_DES rng( 42 );

        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ParasiteGeneticsTest/TestRecombinationLargeBarcode.json" );
        SusceptibilityMalariaConfig::falciparumPfEMP1Vars = 300;

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        // ------------------------------------------------------------------
        // --- Create a base genomes for testing
        // ------------------------------------------------------------------
        int32_t num_snps = ParasiteGenetics::GetInstance()->GetIndexesBarcode().size();
        std::string barcode_1( num_snps, ' ' );
        std::string barcode_2( num_snps, ' ' );
        for( int i = 0; i < num_snps; ++i )
        {
            barcode_1[ i ] = 'A';
            barcode_2[ i ] = 'T';
        }

        std::string drug = "";
        std::string hrp = "";
        int32_t msp_1 = 1;
        std::vector<int32_t> pfemp1_values_1 = {
            100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
            110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
            120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
            130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
            140, 141, 142, 143, 144, 145, 146, 147, 148, 149
        };
        ParasiteGenome pg_tmp_1 = ParasiteGenetics::GetInstance()->CreateGenomeFromSequence( &rng, barcode_1, drug, hrp, msp_1, pfemp1_values_1 );
        ParasiteGenome pg_1 = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_1, 1 );

        CHECK_EQUAL(          2, pg_1.GetID() );
        CHECK_EQUAL(      msp_1, pg_1.GetMSP() );
        CHECK_EQUAL(  barcode_1, pg_1.GetBarcode() );

        std::vector<int32_t> pfemp1_values_actual = pg_1.GetPfEMP1EpitopesMajor();
        CHECK_EQUAL( 50, pfemp1_values_actual.size() ); //CLONAL_PfEMP1_VARIANTS
        CHECK_ARRAY_EQUAL( pfemp1_values_1, pfemp1_values_actual, pfemp1_values_actual.size() );

        int32_t msp_2 = 2;
        std::vector<int32_t> pfemp1_values_2 = {
            200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
            210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
            220, 221, 222, 223, 224, 225, 226, 227, 228, 229,
            230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
            240, 241, 242, 243, 244, 245, 246, 247, 248, 249
        };
        ParasiteGenome pg_tmp_2 = ParasiteGenetics::GetInstance()->CreateGenomeFromSequence( &rng, barcode_2, drug, hrp, msp_2, pfemp1_values_2 );
        ParasiteGenome pg_2 = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_2, 2 );

        CHECK_EQUAL(          4, pg_2.GetID() );
        CHECK_EQUAL(      msp_2, pg_2.GetMSP() );
        CHECK_EQUAL(  barcode_2, pg_2.GetBarcode() );

        pfemp1_values_actual = pg_2.GetPfEMP1EpitopesMajor();
        CHECK_EQUAL( 50, pfemp1_values_actual.size() ); //CLONAL_PfEMP1_VARIANTS
        CHECK_ARRAY_EQUAL( pfemp1_values_2, pfemp1_values_actual, pfemp1_values_actual.size() );

        std::vector<ParasiteGenome> new_genomes;

        for( int i_sample = 0; i_sample < 10000; ++i_sample )
        {
            new_genomes.clear();

            ParasiteGenome::Recombination( &rng, pg_1, pg_2, new_genomes );
            CHECK_EQUAL( 4, new_genomes.size() );

            std::vector<std::string> barcodes;
            for( auto& genome : new_genomes )
            {
                barcodes.push_back( genome.GetBarcode() );
            }

            // verify that at each barcode location there are 2-T's and 2-A's
            for( int i = 0; i < barcode_1.length(); ++i )
            {
                int num_A = 0;
                int num_T = 0;

                for( auto& barcode : barcodes )
                {
                    num_A += (barcode[i] == 'A') ? 1 : 0;
                    num_T += (barcode[i] == 'T') ? 1 : 0;
                }
                CHECK_EQUAL( 2, num_A );
                CHECK_EQUAL( 2, num_T );
            }
        }
    }

    TEST_FIXTURE( ParasiteFixture, TestRecombinationMarkers )
    {
        PSEUDO_DES rng( 42 );

        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ParasiteGeneticsTest/TestRecombinationMarkers.json" );

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        // ------------------------------------------------------------------
        // --- Create a base genomes for testing
        // ------------------------------------------------------------------
        std::string barcode_1 = "ATATATATATATATATATATATATATAT";
        std::string drug = "";
        std::string hrp = "";
        int32_t msp_1 = 1;
        std::vector<int32_t> pfemp1_values_1 = {
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1
        };
        ParasiteGenome pg_tmp_1 = ParasiteGenetics::GetInstance()->CreateGenomeFromSequence( &rng, barcode_1, drug, hrp, msp_1, pfemp1_values_1 );
        ParasiteGenome pg_1 = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_1, 1 );

        CHECK_EQUAL(          2, pg_1.GetID() );
        CHECK_EQUAL(      msp_1, pg_1.GetMSP() );
        CHECK_EQUAL(  barcode_1, pg_1.GetBarcode() );

        std::vector<int32_t> pfemp1_values_actual = pg_1.GetPfEMP1EpitopesMajor();
        CHECK_EQUAL( 50, pfemp1_values_actual.size() ); //CLONAL_PfEMP1_VARIANTS
        CHECK_ARRAY_EQUAL( pfemp1_values_1, pfemp1_values_actual, pfemp1_values_actual.size() );


        std::string barcode_2 = "TATATATATATATATATATATATATATA";
        int32_t msp_2 = 2;
        std::vector<int32_t> pfemp1_values_2 = {
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
            2, 2, 2, 2, 2, 2, 2, 2, 2, 2
        };
        ParasiteGenome pg_tmp_2 = ParasiteGenetics::GetInstance()->CreateGenomeFromSequence( &rng, barcode_2, drug, hrp, msp_2, pfemp1_values_2 );
        ParasiteGenome pg_2 = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_2, 2 );

        CHECK_EQUAL(          4, pg_2.GetID() );
        CHECK_EQUAL(      msp_2, pg_2.GetMSP() );
        CHECK_EQUAL(  barcode_2, pg_2.GetBarcode() );

        pfemp1_values_actual = pg_2.GetPfEMP1EpitopesMajor();
        CHECK_EQUAL( 50, pfemp1_values_actual.size() ); //CLONAL_PfEMP1_VARIANTS
        CHECK_ARRAY_EQUAL( pfemp1_values_2, pfemp1_values_actual, pfemp1_values_actual.size() );

        std::vector<ParasiteGenome> new_genomes;

        for( int i_sample = 0; i_sample < 10000; ++i_sample )
        {
            new_genomes.clear();

            // --------------------------------------------------------------------------------
            // --- The config file puts the barcode markers at the beginning and end of each
            // --- chromosome.  The Crossover_Gamma_K is large so that there is only the obligate
            // --- crossover on each chromosome.  This means for each chromosome there should be
            // --- one crossover event between one pair of chromatids.  Since the markers are at
            // --- the ends, the first one will stay put but the one on the far right will change
            // --- chromatids due to the crossover.  This implies for each chromosome we should
            // --- get two chromosomes with a pair of either AA or TT.  The other pair will be
            // --- the original AT or TA.
            // --------------------------------------------------------------------------------
            ParasiteGenome::Recombination( &rng, pg_1, pg_2, new_genomes );
            CHECK_EQUAL( 4, new_genomes.size() );

            std::vector<std::string> barcodes;
            for( auto& genome : new_genomes )
            {
                //printf("%s\n",genome.GetBarcode().c_str());
                barcodes.push_back( genome.GetBarcode() );
            }

            for( int i = 0; (i+1) < barcode_1.length(); i+=2 )
            {
                int num_AA = 0;
                int num_TT = 0;
                int num_AT = 0;
                int num_TA = 0;
                for( int j = 0; j < barcodes.size(); ++j )
                {
                    num_AA += (barcodes[j][i] =='A') && (barcodes[j][i+1] =='A') ? 1 : 0;
                    num_AT += (barcodes[j][i] =='A') && (barcodes[j][i+1] =='T') ? 1 : 0;
                    num_TA += (barcodes[j][i] =='T') && (barcodes[j][i+1] =='A') ? 1 : 0;
                    num_TT += (barcodes[j][i] =='T') && (barcodes[j][i+1] =='T') ? 1 : 0;
                }
                CHECK_EQUAL( 1, num_AA );
                CHECK_EQUAL( 1, num_AT );
                CHECK_EQUAL( 1, num_TA );
                CHECK_EQUAL( 1, num_TT );
            }

            // ----------------------------------------------------------------------------
            // --- To show Independent Assortment, we want to see the odd number locations
            // --- (far left marker) to have a different value since they are most likely
            // --- to change due to independent assortment.  All we can do is verify that it
            // --- does occur.  IA could not change things for a individual chromosome,
            // --- but it should occur at least once in the 14.
            // ----------------------------------------------------------------------------
        
            bool detected_IA = false;
            for( int i = 0; !detected_IA && ((i+1) < barcode_1.length()); i+=2 )
            {
                detected_IA |= (barcodes[0][i] == 'T');
                detected_IA |= (barcodes[1][i] == 'T');
                detected_IA |= (barcodes[2][i] == 'A');
                detected_IA |= (barcodes[3][i] == 'A');
            }
            CHECK( detected_IA );
        }
    }

    TEST_FIXTURE( ParasiteFixture, TestRecombinationOrderBug )
    {
        PSEUDO_DES rng( 42 );

        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ParasiteGeneticsTest/TestRecombinationOrderBug.json" );

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        // ------------------------------------------------------------------
        // --- Create a base genomes for testing
        // ------------------------------------------------------------------
        std::string drug_1 = "TAT";
        std::string hrp_1  = "CG";
        std::string barcode_1 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
        ParasiteGenome pg_tmp_1 = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode_1, drug_1, hrp_1 );
        ParasiteGenome pg_1 = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_1, 1 );

        CHECK_EQUAL(          2, pg_1.GetID() );
        CHECK_EQUAL(  barcode_1, pg_1.GetBarcode() );
        CHECK_EQUAL(     drug_1, pg_1.GetDrugResistantString() );
        CHECK_EQUAL(      hrp_1, pg_1.GetHrpString() );


        std::string drug_2 = "ATA";
        std::string hrp_2  = "GC";
        std::string barcode_2 = "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT";
        ParasiteGenome pg_tmp_2 = ParasiteGenetics::GetInstance()->CreateGenomeFromBarcode( &rng, barcode_2, drug_2, hrp_2 );
        ParasiteGenome pg_2 = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_2, 2 );

        CHECK_EQUAL(          4, pg_2.GetID() );
        CHECK_EQUAL(  barcode_2, pg_2.GetBarcode() );
        CHECK_EQUAL(     drug_2, pg_2.GetDrugResistantString() );
        CHECK_EQUAL(      hrp_2, pg_2.GetHrpString() );

        std::list<Crossover> crossovers;
        crossovers.push_back( Crossover( 1, 0,   60000 ) );
        crossovers.push_back( Crossover( 0, 0,  320000 ) );
        crossovers.push_back( Crossover( 0, 1, 3000001 ) ); // not part of original bug
        ParasiteGenome::TEST_SetCrossovers( crossovers );

        std::vector<ParasiteGenome> new_genomes;
        ParasiteGenome::Recombination( &rng, pg_1, pg_2, new_genomes );
        CHECK_EQUAL( 4, new_genomes.size() );

        std::string exp_barcode_0 = "TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT";
        std::string exp_barcode_1 = "AAAAAAAAAATTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
        std::string exp_barcode_2 = "TTTTTTTTTTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
        std::string exp_barcode_3 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAATTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT";

        CHECK_EQUAL( exp_barcode_0, new_genomes[0].GetBarcode() );
        CHECK_EQUAL( exp_barcode_1, new_genomes[1].GetBarcode() );
        CHECK_EQUAL( exp_barcode_2, new_genomes[2].GetBarcode() );
        CHECK_EQUAL( exp_barcode_3, new_genomes[3].GetBarcode() );

        std::string exp_drug_0 = "ATA";
        std::string exp_drug_1 = "TTT"; // both crossovers
        std::string exp_drug_2 = "AAT"; // first crossover
        std::string exp_drug_3 = "TAA"; // second crossover

        CHECK_EQUAL( exp_drug_0, new_genomes[ 0 ].GetDrugResistantString() );
        CHECK_EQUAL( exp_drug_1, new_genomes[ 1 ].GetDrugResistantString() );
        CHECK_EQUAL( exp_drug_2, new_genomes[ 2 ].GetDrugResistantString() );
        CHECK_EQUAL( exp_drug_3, new_genomes[ 3 ].GetDrugResistantString() );

        // -----------------------------
        // --- not part of original bug
        // -----------------------------
        std::string exp_hrp_0 = "CC";
        std::string exp_hrp_1 = "GG";
        std::string exp_hrp_2 = "CG";
        std::string exp_hrp_3 = "GC";

        CHECK_EQUAL( exp_hrp_0, new_genomes[ 0 ].GetHrpString() );
        CHECK_EQUAL( exp_hrp_1, new_genomes[ 1 ].GetHrpString() );
        CHECK_EQUAL( exp_hrp_2, new_genomes[ 2 ].GetHrpString() );
        CHECK_EQUAL( exp_hrp_3, new_genomes[ 3 ].GetHrpString() );
    }

    TEST_FIXTURE( ParasiteFixture, TestFindPossibleBarcodeHashcodes )
    {
        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ParasiteGeneticsTest/TestConvertLocationToIndex.json" );

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        // ------------------------------------------------------------------
        // --- Test a fix barcode
        // ------------------------------------------------------------------
        std::string barcode = "AAAAAAAAAAAAAAAAAAAAAAAA";

        std::vector<int64_t> hashcodes = ParasiteGenetics::GetInstance()->FindPossibleBarcodeHashcodes( "test", barcode );

        CHECK_EQUAL( 1, hashcodes.size() );
        CHECK_EQUAL( 5572157665409572113, hashcodes[ 0 ] );

        // ------------------------------------------------------------------
        // --- Test a different fix barcode
        // ------------------------------------------------------------------
        barcode = "ACGTACGTACGTACGTACGTACGT";

        hashcodes = ParasiteGenetics::GetInstance()->FindPossibleBarcodeHashcodes( "test", barcode );

        CHECK_EQUAL( 1, hashcodes.size() );
        CHECK_EQUAL( 4326324628502345245, hashcodes[ 0 ] );

        // ------------------------------------------------------------------
        // --- Test barcode with one wild card
        // ------------------------------------------------------------------
        barcode = "A*AAAAAAAAAAAAAAAAAAAAAA";

        hashcodes = ParasiteGenetics::GetInstance()->FindPossibleBarcodeHashcodes( "test", barcode );

        CHECK_EQUAL( 4, hashcodes.size() );
        CHECK_EQUAL(  5572157665409572113, hashcodes[ 0 ] );
        CHECK_EQUAL(  7716733729169126994, hashcodes[ 1 ] );
        CHECK_EQUAL( -8585434280780869741, hashcodes[ 2 ] );
        CHECK_EQUAL( -6440858217021314860, hashcodes[ 3 ] );

        // ------------------------------------------------------------------
        // --- Test barcode with two wild cards
        // ------------------------------------------------------------------
        barcode = "A*T*AAAAAAAAAAAAAAAAAAAA";

        hashcodes = ParasiteGenetics::GetInstance()->FindPossibleBarcodeHashcodes( "test", barcode );

        CHECK_EQUAL( 16, hashcodes.size() );
        CHECK_EQUAL(  3399472242714103022, hashcodes[  0 ] );
        CHECK_EQUAL(  5544048306473657903, hashcodes[  1 ] );
        CHECK_EQUAL(  7688624370233212784, hashcodes[  2 ] );
        CHECK_EQUAL( -8613543639716783951, hashcodes[  3 ] );
        CHECK_EQUAL(  4169518381124239983, hashcodes[  4 ] );
        CHECK_EQUAL(  6314094444883794864, hashcodes[  5 ] );
        CHECK_EQUAL(  8458670508643349745, hashcodes[  6 ] );
        CHECK_EQUAL( -7843497501306646990, hashcodes[  7 ] );
        CHECK_EQUAL(  4939564519534376944, hashcodes[  8 ] );
        CHECK_EQUAL(  7084140583293931825, hashcodes[  9 ] );
        CHECK_EQUAL( -9218027426656064910, hashcodes[ 10 ] );
        CHECK_EQUAL( -7073451362896510029, hashcodes[ 11 ] );
        CHECK_EQUAL(  5709610657944513905, hashcodes[ 12 ] );
        CHECK_EQUAL(  7854186721704068786, hashcodes[ 13 ] );
        CHECK_EQUAL( -8447981288245927949, hashcodes[ 14 ] );
        CHECK_EQUAL( -6303405224486373068, hashcodes[ 15 ] );

        // ------------------------------------------------------------------
        // --- Test barcode with wild card in first spot
        // ------------------------------------------------------------------
        barcode = "*AAAAAAAAAAAAAAAAAAAAAAA";

        hashcodes = ParasiteGenetics::GetInstance()->FindPossibleBarcodeHashcodes( "test", barcode );

        CHECK_EQUAL( 4, hashcodes.size() );
        CHECK_EQUAL(  5572157665409572113, hashcodes[  0 ] );
        CHECK_EQUAL( -1732960652882433040, hashcodes[  1 ] );
        CHECK_EQUAL( -9038078971174438193, hashcodes[  2 ] );
        CHECK_EQUAL(  2103546784243108270, hashcodes[  3 ] );
    }
#endif

    TEST_FIXTURE( ParasiteFixture, TestSerialization )
    {
        PSEUDO_DES rng( 42 );

        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/ParasiteGeneticsTest/TestConvertLocationToIndex.json" );
        SusceptibilityMalariaConfig::falciparumPfEMP1Vars = 300;

        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );

        // ------------------------------------------------------------------
        // --- Create a base genomes for testing
        // ------------------------------------------------------------------
        std::string barcode_1 = "ACGTTGCAACGTTGCAACGTTGCA";
        std::string drug_1 = "TAT";
        std::string hrp_1 = "C";
        int32_t msp_1 = 1;
        std::vector<int32_t> pfemp1_values_1 = {
            100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
            110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
            120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
            130, 131, 132, 133, 134, 135, 136, 137, 138, 139,
            140, 141, 142, 143, 144, 145, 146, 147, 148, 149
        };
        ParasiteGenome pg_tmp_1 = ParasiteGenetics::GetInstance()->CreateGenomeFromSequence( &rng, barcode_1, drug_1, hrp_1, msp_1, pfemp1_values_1 );
        ParasiteGenome pg_exp_1 = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_1, 1 );

        CHECK_EQUAL(          2, pg_exp_1.GetID() );
        CHECK_EQUAL(      msp_1, pg_exp_1.GetMSP() );
        CHECK_EQUAL(  barcode_1, pg_exp_1.GetBarcode() );
        CHECK_EQUAL(     drug_1, pg_exp_1.GetDrugResistantString() );
        CHECK_EQUAL(      hrp_1, pg_exp_1.GetHrpString() );

        std::vector<int32_t> pfemp1_values_actual = pg_exp_1.GetPfEMP1EpitopesMajor();
        CHECK_EQUAL( 50, pfemp1_values_actual.size() ); //CLONAL_PfEMP1_VARIANTS
        CHECK_ARRAY_EQUAL( pfemp1_values_1, pfemp1_values_actual, pfemp1_values_actual.size() );

        std::string barcode_2 = "TTTTTTTTTTTTTTTTTTTTTTTT";
        std::string drug_2 = "ATA";
        std::string hrp_2 = "G";
        int32_t msp_2 = 2;
        std::vector<int32_t> pfemp1_values_2 = {
            200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
            210, 211, 212, 213, 214, 215, 216, 217, 218, 219,
            220, 221, 222, 223, 224, 225, 226, 227, 228, 229,
            230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
            240, 241, 242, 243, 244, 245, 246, 247, 248, 249
        };
        ParasiteGenome pg_tmp_2 = ParasiteGenetics::GetInstance()->CreateGenomeFromSequence( &rng, barcode_2, drug_2, hrp_2, msp_2, pfemp1_values_2 );
        ParasiteGenome pg_exp_2 = ParasiteGenetics::GetInstance()->CreateGenome( pg_tmp_2, 2 );

        CHECK_EQUAL(          4, pg_exp_2.GetID() );
        CHECK_EQUAL(      msp_2, pg_exp_2.GetMSP() );
        CHECK_EQUAL(  barcode_2, pg_exp_2.GetBarcode() );
        CHECK_EQUAL(     drug_2, pg_exp_2.GetDrugResistantString() );
        CHECK_EQUAL(      hrp_2, pg_exp_2.GetHrpString() );

        pfemp1_values_actual = pg_exp_2.GetPfEMP1EpitopesMajor();
        CHECK_EQUAL( 50, pfemp1_values_actual.size() ); //CLONAL_PfEMP1_VARIANTS
        CHECK_ARRAY_EQUAL( pfemp1_values_2, pfemp1_values_actual, pfemp1_values_actual.size() );

        // -------------------------------
        // --- Save the data to the writer
        // -------------------------------
        JsonFullWriter json_writer;
        IArchive* p_json_writer = &json_writer;
        p_json_writer->labelElement("ParasiteGenetics"); ParasiteGenetics::serialize( *p_json_writer );
        p_json_writer->labelElement("Genome1") & pg_exp_1;
        p_json_writer->labelElement("Genome2") & pg_exp_2;

        // ------------------------------------------------------------
        // --- Remove the instance of the ParasiteGenetics and the map
        // ------------------------------------------------------------
        ParasiteGenetics::DeleteInstance();
        ParasiteGenetics::CreateInstance()->Configure( EnvPtr->Config );
        ParasiteGenome pg_act_1;
        ParasiteGenome pg_act_2;

        // --------------------------------------------------------------------------------------------------
        // --- Read the data back into objects and verify the objects read equal the ones that were written.
        // --------------------------------------------------------------------------------------------------
        JsonFullReader json_reader( p_json_writer->GetBuffer() );
        IArchive* p_json_reader = &json_reader;
        p_json_reader->labelElement("ParasiteGenetics"); ParasiteGenetics::serialize( *p_json_reader );
        p_json_reader->labelElement("Genome1") & pg_act_1;
        p_json_reader->labelElement("Genome2") & pg_act_2;

        CHECK_EQUAL( pg_exp_1.GetID(),                  pg_act_1.GetID() );
        CHECK_EQUAL( pg_exp_1.GetBarcodeHashcode(),     pg_act_1.GetBarcodeHashcode() );
        CHECK_EQUAL( pg_exp_1.GetBarcode(),             pg_act_1.GetBarcode() );
        CHECK_EQUAL( pg_exp_1.GetDrugResistantString(), pg_act_1.GetDrugResistantString() );
        CHECK_EQUAL( pg_exp_1.GetHrpString(),           pg_act_1.GetHrpString() );

        CHECK_ARRAY_EQUAL( pg_exp_1.GetNucleotideSequence(), pg_act_1.GetNucleotideSequence(), pg_exp_1.GetNucleotideSequence().size() );
        CHECK_ARRAY_EQUAL( pg_exp_1.GetAlleleRoots(),        pg_act_1.GetAlleleRoots(),        pg_exp_1.GetAlleleRoots().size() );

        CHECK_EQUAL( pg_exp_2.GetID(),                  pg_act_2.GetID() );
        CHECK_EQUAL( pg_exp_2.GetBarcodeHashcode(),     pg_act_2.GetBarcodeHashcode() );
        CHECK_EQUAL( pg_exp_2.GetBarcode(),             pg_act_2.GetBarcode() );
        CHECK_EQUAL( pg_exp_2.GetDrugResistantString(), pg_act_2.GetDrugResistantString() );
        CHECK_EQUAL( pg_exp_2.GetHrpString(),           pg_act_2.GetHrpString() );

        CHECK_ARRAY_EQUAL( pg_exp_2.GetNucleotideSequence(), pg_act_2.GetNucleotideSequence(), pg_exp_2.GetNucleotideSequence().size() );
        CHECK_ARRAY_EQUAL( pg_exp_2.GetAlleleRoots(),        pg_act_2.GetAlleleRoots(),        pg_exp_2.GetAlleleRoots().size() );
    }

#if 1
    TEST_FIXTURE( ParasiteFixture, TestInvalidPfEMP1Size )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/ParasiteGeneticsTest/TestInvalidPfEMP1Size.json",
                                       "'PfEMP1_Variants_Genome_Locations' must define exactly 50 locations." );
    }

    TEST_FIXTURE( ParasiteFixture, TestDupliateValuesInDrugResistantLocations )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/ParasiteGeneticsTest/TestDupliateValuesInDrugResistantLocations.json",
                                       "The values in 'Drug_Resistant_Genome_Locations' must be unique and in ascending order." );
    }

    TEST_FIXTURE( ParasiteFixture, TestDupliateValuesInBarcodeLocations )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/ParasiteGeneticsTest/TestDupliateValuesInBarcodeLocations.json",
                                       "The values in 'Barcode_Genome_Locations' must be unique and in ascending order." );
    }

    TEST_FIXTURE( ParasiteFixture, TestDupliateValuesInPfEMP1Locations )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/ParasiteGeneticsTest/TestDupliateValuesInPfEMP1Locations.json",
                                       "The values in 'PfEMP1_Variants_Genome_Locations' must be unique and in ascending order." );
    }

    TEST_FIXTURE( ParasiteFixture, TestMSPLocationInBarcodeLocations )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/ParasiteGeneticsTest/TestMSPLocationInBarcodeLocations.json",
                                       "Invalid configuration for parameters 'Barcode_Genome_Locations' and 'MSP_Genome_Location' - duplicate values.\nBarcode_Genome_Locations[ 14 ] = MSP_Genome_Location = 140" );
    }

    TEST_FIXTURE( ParasiteFixture, TestMSPLocationInPfEMP1Locations )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/ParasiteGeneticsTest/TestMSPLocationInPfEMP1Locations.json",
                                       "Invalid configuration for parameters 'PfEMP1_Variants_Genome_Locations' and 'MSP_Genome_Location' - duplicate values.\nPfEMP1_Variants_Genome_Locations[ 31 ] = MSP_Genome_Location = 1201" );
    }

    TEST_FIXTURE( ParasiteFixture, TestBarcodeLocationInDrugResistantLocations )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/ParasiteGeneticsTest/TestBarcodeLocationInDrugResistantLocations.json",
                                       "Invalid configuration for parameters 'Barcode_Genome_Locations' and 'Drug_Resistant_Genome_Locations' - duplicate values.\nBarcode_Genome_Locations[ 2 ] = Drug_Resistant_Genome_Locations[ 1 ] = 20" );
    }

    TEST_FIXTURE( ParasiteFixture, TestMSPLocationInDrugResistantLocations )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/ParasiteGeneticsTest/TestMSPLocationInDrugResistantLocations.json",
                                       "Invalid configuration for parameters 'Drug_Resistant_Genome_Locations' and 'MSP_Genome_Location' - duplicate values.\nDrug_Resistant_Genome_Locations[ 1 ] = MSP_Genome_Location = 2001" );
    }

    TEST_FIXTURE( ParasiteFixture, TestPfEMP1LocationInDrugResistantLocations )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/ParasiteGeneticsTest/TestPfEMP1LocationInDrugResistantLocations.json",
                                       "Invalid configuration for parameters 'PfEMP1_Variants_Genome_Locations' and 'Drug_Resistant_Genome_Locations' - duplicate values.\nPfEMP1_Variants_Genome_Locations[ 49 ] = Drug_Resistant_Genome_Locations[ 1 ] = 2001" );
    }

    TEST_FIXTURE( ParasiteFixture, TestBarcodeLocationInHrpLocations )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/ParasiteGeneticsTest/TestBarcodeLocationInHrpLocations.json",
                                       "Invalid configuration for parameters 'Barcode_Genome_Locations' and 'HRP_Genome_Locations' - duplicate values.\nBarcode_Genome_Locations[ 3 ] = HRP_Genome_Locations[ 1 ] = 30" );
    }

    TEST_FIXTURE( ParasiteFixture, TestDrugLocationInHrpLocations )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/ParasiteGeneticsTest/TestDrugLocationInHrpLocations.json",
                                       "Invalid configuration for parameters 'HRP_Genome_Locations' and 'Drug_Resistant_Genome_Locations' - duplicate values.\nHRP_Genome_Locations[ 0 ] = Drug_Resistant_Genome_Locations[ 0 ] = 2000" );
    }

    TEST_FIXTURE( ParasiteFixture, TestPfEMP1LocationInHrpLocations )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/ParasiteGeneticsTest/TestPfEMP1LocationInHrpLocations.json",
                                       "Invalid configuration for parameters 'PfEMP1_Variants_Genome_Locations' and 'HRP_Genome_Locations' - duplicate values.\nPfEMP1_Variants_Genome_Locations[ 44 ] = HRP_Genome_Locations[ 0 ] = 1304" );
    }

    TEST_FIXTURE( ParasiteFixture, TestMSPLocationInHrpLocations )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/ParasiteGeneticsTest/TestMspLocationInHrpLocations.json",
                                       "Invalid configuration for parameters 'HRP_Genome_Locations' and 'MSP_Genome_Location' - duplicate values.\nHRP_Genome_Locations[ 0 ] = MSP_Genome_Location = 77" );
    }

    TEST_FIXTURE( ParasiteFixture, TestInvalidNeighborhoodSizeMSP )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/ParasiteGeneticsTest/TestInvalidNeighborhoodSizeMSP.json",
                                       "Variable or parameter 'Neighborhood_Size_MSP' with value 333 is incompatible with variable or parameter 'Falciparum_MSP_Variants' with value 100. \n'Neighborhood_Size_MSP' must <= to 'Falciparum_MSP_Variants'" );
    }

    TEST_FIXTURE( ParasiteFixture, TestInvalidNeighborhoodSizePfEMP1 )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/ParasiteGeneticsTest/TestInvalidNeighborhoodSizePfEMP1.json",
                                       "Variable or parameter 'Neighborhood_Size_PfEMP1' with value 4444 is incompatible with variable or parameter 'Falciparum_PfEMP1_Variants' with value 1000. \n'Neighborhood_Size_PfEMP1' must <= to 'Falciparum_PfEMP1_Variants'" );
    }
#endif
}