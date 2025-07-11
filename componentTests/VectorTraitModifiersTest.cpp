
#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"
#include "VectorTraitModifiers.h"
#include "VectorGene.h"
#include "SusceptibilityMalaria.h"
#include "ParasiteGenetics.h"
#include "ParasiteGenome.h"

using namespace Kernel;

SUITE( VectorTraitModifiersTest )
{
    struct VectorTraitModifiersFixture
    {
        VectorTraitModifiersFixture()
        {
            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = false;
            JsonConfigurable::_track_missing = false;

            SusceptibilityMalariaConfig::falciparumMSPVars      = DEFAULT_MSP_VARIANTS;
            SusceptibilityMalariaConfig::falciparumNonSpecTypes = DEFAULT_NONSPECIFIC_TYPES;
            SusceptibilityMalariaConfig::falciparumPfEMP1Vars   = DEFAULT_PFEMP1_VARIANTS;
        }

        ~VectorTraitModifiersFixture()
        {
            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = true;
            JsonConfigurable::_track_missing = true;

            ParasiteGenetics::CreateInstance()->ReduceGenomeMap();
            ParasiteGenetics::DeleteInstance();
            Environment::Finalize();
        }
    };

    TEST_FIXTURE( VectorTraitModifiersFixture, TestConfigure )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/VectorTraitModifiersTest/TestConfigure.json" ) );

        VectorGeneCollection gene_collection;
        try
        {
            gene_collection.ConfigureFromJsonAndKey( p_config.get(), "Genes" );
            gene_collection.CheckConfiguration();
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        VectorTraitModifiers trait_modifiers( &gene_collection );
        try
        {
            trait_modifiers.ConfigureFromJsonAndKey( p_config.get(), "Gene_To_Trait_Modifiers" );
            trait_modifiers.CheckConfiguration();
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        CHECK_EQUAL( 6, gene_collection.Size() );
        CHECK_EQUAL( 6, trait_modifiers.Size() );

        VectorGenome genome_modified_1;
        genome_modified_1.SetLocus( 0, 0, 0 ); // female
        genome_modified_1.SetLocus( 1, 1, 0 ); // a2,a1
        genome_modified_1.SetLocus( 2, 0, 0 ); // b1,b1**
        genome_modified_1.SetLocus( 3, 1, 1 ); // c2,c2**
        genome_modified_1.SetLocus( 4, 0, 1 ); // d1,d2**
        genome_modified_1.SetLocus( 5, 1, 0 ); // e2,e1
        CHECK_EQUAL( "X-a2-b1-c2-d1-e2:X-a1-b1-c2-d2-e1", gene_collection.GetGenomeName( genome_modified_1 ) );

        VectorGenome genome_modified_2;
        genome_modified_2.SetLocus( 0, 0, 4 ); // male
        genome_modified_2.SetLocus( 1, 1, 1 ); // a2,a2
        genome_modified_2.SetLocus( 2, 1, 1 ); // b2,b2
        genome_modified_2.SetLocus( 3, 1, 2 ); // c2,c3**
        genome_modified_2.SetLocus( 4, 1, 1 ); // d2,d2
        genome_modified_2.SetLocus( 5, 0, 1 ); // e1,e2**
        CHECK_EQUAL( "X-a2-b2-c2-d2-e1:Y-a2-b2-c3-d2-e2", gene_collection.GetGenomeName( genome_modified_2 ) );

        VectorGenome genome_modified_3;
        genome_modified_3.SetLocus( 0, 0, 0 ); // female
        genome_modified_3.SetLocus( 1, 1, 0 ); // a2,a1
        genome_modified_3.SetLocus( 2, 0, 4 ); // b1,b5**
        genome_modified_3.SetLocus( 3, 2, 1 ); // c3,c2
        genome_modified_3.SetLocus( 4, 0, 0 ); // d1,d1
        genome_modified_3.SetLocus( 5, 1, 1 ); // e2,e2
        CHECK_EQUAL( "X-a2-b1-c3-d1-e2:X-a1-b5-c2-d1-e2", gene_collection.GetGenomeName( genome_modified_3 ) );

        VectorGenome genome_modified_4;
        genome_modified_4.SetLocus( 0, 0, 4 ); // male
        genome_modified_4.SetLocus( 1, 0, 0 ); // a1,a1
        genome_modified_4.SetLocus( 2, 2, 3 ); // b3,b4**
        genome_modified_4.SetLocus( 3, 0, 0 ); // c1,c1
        genome_modified_4.SetLocus( 4, 0, 0 ); // d1,d1
        genome_modified_4.SetLocus( 5, 1, 1 ); // e2,e2
        CHECK_EQUAL( "X-a1-b3-c1-d1-e2:Y-a1-b4-c1-d1-e2", gene_collection.GetGenomeName( genome_modified_4 ) );

        VectorGenome genome_modified_4b;
        genome_modified_4b.SetLocus( 0, 0, 4 ); // male
        genome_modified_4b.SetLocus( 1, 0, 0 ); // a1,a1
        genome_modified_4b.SetLocus( 2, 3, 2 ); // b4**,b3
        genome_modified_4b.SetLocus( 3, 0, 0 ); // c1,c1
        genome_modified_4b.SetLocus( 4, 0, 0 ); // d1,d1
        genome_modified_4b.SetLocus( 5, 1, 1 ); // e2,e2
        CHECK_EQUAL( "X-a1-b4-c1-d1-e2:Y-a1-b3-c1-d1-e2", gene_collection.GetGenomeName( genome_modified_4b ) );

        VectorGenome genome_unmodified;
        genome_unmodified.SetLocus( 0, 0, 4 ); // male
        genome_unmodified.SetLocus( 1, 0, 0 ); // a1,a1
        genome_unmodified.SetLocus( 2, 1, 1 ); // b2,b2
        genome_unmodified.SetLocus( 3, 0, 0 ); // c1,c1
        genome_unmodified.SetLocus( 4, 0, 0 ); // d1,d1
        genome_unmodified.SetLocus( 5, 1, 1 ); // e2,e2
        CHECK_EQUAL( "X-a1-b2-c1-d1-e2:Y-a1-b2-c1-d1-e2", gene_collection.GetGenomeName( genome_unmodified ) );

        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::INFECTED_BY_HUMAN,     genome_unmodified ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::FECUNDITY,             genome_unmodified ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::FEMALE_EGG_RATIO,      genome_unmodified ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::STERILITY,             genome_unmodified ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::TRANSMISSION_TO_HUMAN, genome_unmodified ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::ADJUST_FERTILE_EGGS,   genome_unmodified ), FLT_EPSILON );

        CHECK_CLOSE( 1.5, trait_modifiers.GetModifier( VectorTrait::INFECTED_BY_HUMAN,     genome_modified_1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.9, trait_modifiers.GetModifier( VectorTrait::FECUNDITY,             genome_modified_1 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::FEMALE_EGG_RATIO,      genome_modified_1 ), FLT_EPSILON );
        CHECK_CLOSE( 0.2, trait_modifiers.GetModifier( VectorTrait::STERILITY,             genome_modified_1 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::TRANSMISSION_TO_HUMAN, genome_modified_1 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::ADJUST_FERTILE_EGGS,   genome_modified_1 ), FLT_EPSILON );

        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::INFECTED_BY_HUMAN,     genome_modified_2 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::FECUNDITY,             genome_modified_2 ), FLT_EPSILON );
        CHECK_CLOSE( 0.5, trait_modifiers.GetModifier( VectorTrait::FEMALE_EGG_RATIO,      genome_modified_2 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::STERILITY,             genome_modified_2 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::TRANSMISSION_TO_HUMAN, genome_modified_2 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::ADJUST_FERTILE_EGGS,   genome_modified_2 ), FLT_EPSILON );

        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::INFECTED_BY_HUMAN,     genome_modified_3 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::FECUNDITY,             genome_modified_3 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::FEMALE_EGG_RATIO,      genome_modified_3 ), FLT_EPSILON );
        CHECK_CLOSE( 0.0, trait_modifiers.GetModifier( VectorTrait::STERILITY,             genome_modified_3 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::TRANSMISSION_TO_HUMAN, genome_modified_3 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::ADJUST_FERTILE_EGGS,   genome_modified_3 ), FLT_EPSILON );

        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::INFECTED_BY_HUMAN,     genome_modified_4 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::FECUNDITY,             genome_modified_4 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::FEMALE_EGG_RATIO,      genome_modified_4 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::STERILITY,             genome_modified_4 ), FLT_EPSILON );
        CHECK_CLOSE( 0.7, trait_modifiers.GetModifier( VectorTrait::TRANSMISSION_TO_HUMAN, genome_modified_4 ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::ADJUST_FERTILE_EGGS,   genome_modified_4 ), FLT_EPSILON );

        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::INFECTED_BY_HUMAN,     genome_modified_4b ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::FECUNDITY,             genome_modified_4b ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::FEMALE_EGG_RATIO,      genome_modified_4b ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::STERILITY,             genome_modified_4b ), FLT_EPSILON );
        CHECK_CLOSE( 0.7, trait_modifiers.GetModifier( VectorTrait::TRANSMISSION_TO_HUMAN, genome_modified_4b ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::ADJUST_FERTILE_EGGS,   genome_modified_4b ), FLT_EPSILON );

        VectorGenome genome_non_fertile;
        genome_non_fertile.SetLocus( 0, 0, 1 ); // male
        genome_non_fertile.SetLocus( 1, 0, 0 ); // a1,a1
        genome_non_fertile.SetLocus( 2, 5, 5 ); // b6,b6
        genome_non_fertile.SetLocus( 3, 0, 0 ); // c1,c1
        genome_non_fertile.SetLocus( 4, 0, 0 ); // d1,d1
        genome_non_fertile.SetLocus( 5, 1, 1 ); // e2,e2
        CHECK_CLOSE( 0.5, trait_modifiers.GetModifier( VectorTrait::ADJUST_FERTILE_EGGS, genome_non_fertile ), FLT_EPSILON );

        // -------------------------------------------------------------------------------------------
        // --- The following is to thest that [ X2, * ] detects [ X, X2 ], [ X2, X ], and [ X2, X2 ]
        // --- There was a bug in the original implementation for the gender gene that would not have
        // --- detected [ X, X2 ].
        // -------------------------------------------------------------------------------------------
        VectorGenome genome_modified_5a;
        genome_modified_5a.SetLocus( 0, 0, 1 ); // X-X2
        genome_modified_5a.SetLocus( 1, 0, 0 ); // a1,a1
        genome_modified_5a.SetLocus( 2, 1, 1 ); // b2,b2
        genome_modified_5a.SetLocus( 3, 0, 0 ); // c1,c1
        genome_modified_5a.SetLocus( 4, 0, 0 ); // d1,d1
        genome_modified_5a.SetLocus( 5, 1, 1 ); // e2,e2
        CHECK_EQUAL( "X-a1-b2-c1-d1-e2:X2-a1-b2-c1-d1-e2", gene_collection.GetGenomeName( genome_modified_5a ) );

        VectorGenome genome_modified_5b;
        genome_modified_5b.SetLocus( 0, 1, 0 ); // X2-X
        genome_modified_5b.SetLocus( 1, 0, 0 ); // a1,a1
        genome_modified_5b.SetLocus( 2, 1, 1 ); // b2,b2
        genome_modified_5b.SetLocus( 3, 0, 0 ); // c1,c1
        genome_modified_5b.SetLocus( 4, 0, 0 ); // d1,d1
        genome_modified_5b.SetLocus( 5, 1, 1 ); // e2,e2
        CHECK_EQUAL( "X2-a1-b2-c1-d1-e2:X-a1-b2-c1-d1-e2", gene_collection.GetGenomeName( genome_modified_5b ) );

        VectorGenome genome_modified_5c;
        genome_modified_5c.SetLocus( 0, 1, 1 ); // X2-X2
        genome_modified_5c.SetLocus( 1, 0, 0 ); // a1,a1
        genome_modified_5c.SetLocus( 2, 1, 1 ); // b2,b2
        genome_modified_5c.SetLocus( 3, 0, 0 ); // c1,c1
        genome_modified_5c.SetLocus( 4, 0, 0 ); // d1,d1
        genome_modified_5c.SetLocus( 5, 1, 1 ); // e2,e2
        CHECK_EQUAL( "X2-a1-b2-c1-d1-e2:X2-a1-b2-c1-d1-e2", gene_collection.GetGenomeName( genome_modified_5c ) );

        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::INFECTED_BY_HUMAN,     genome_modified_5a ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::FECUNDITY,             genome_modified_5a ), FLT_EPSILON );
        CHECK_CLOSE( 0.4, trait_modifiers.GetModifier( VectorTrait::FEMALE_EGG_RATIO,      genome_modified_5a ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::STERILITY,             genome_modified_5a ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::TRANSMISSION_TO_HUMAN, genome_modified_5a ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::ADJUST_FERTILE_EGGS,   genome_modified_5a ), FLT_EPSILON );

        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::INFECTED_BY_HUMAN,     genome_modified_5b ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::FECUNDITY,             genome_modified_5b ), FLT_EPSILON );
        CHECK_CLOSE( 0.4, trait_modifiers.GetModifier( VectorTrait::FEMALE_EGG_RATIO,      genome_modified_5b ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::STERILITY,             genome_modified_5b ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::TRANSMISSION_TO_HUMAN, genome_modified_5b ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::ADJUST_FERTILE_EGGS,   genome_modified_5b ), FLT_EPSILON );

        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::INFECTED_BY_HUMAN,     genome_modified_5c ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::FECUNDITY,             genome_modified_5c ), FLT_EPSILON );
        CHECK_CLOSE( 0.4, trait_modifiers.GetModifier( VectorTrait::FEMALE_EGG_RATIO,      genome_modified_5c ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::STERILITY,             genome_modified_5c ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::TRANSMISSION_TO_HUMAN, genome_modified_5c ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::ADJUST_FERTILE_EGGS,   genome_modified_5c ), FLT_EPSILON );
    }

    void TestHelper_ConfigureException( int lineNumber, const std::string& rFilename, const std::string& rExpMsg, bool includeParasiteGenetics=false )
    {
        try
        {
            EnvPtr->Config = Environment::LoadConfigurationFile( rFilename );

            if( includeParasiteGenetics )
            {
                ParasiteGenetics::CreateInstance()->Configure( Environment::CopyFromElement( (*EnvPtr->Config)[ "Parasite_Genetics" ]) );
            }

            VectorGeneCollection gene_collection;
            gene_collection.ConfigureFromJsonAndKey( EnvPtr->Config, "Genes" );
            gene_collection.CheckConfiguration();

            VectorTraitModifiers trait_modifiers( &gene_collection );
            trait_modifiers.ConfigureFromJsonAndKey( EnvPtr->Config, "Gene_To_Trait_Modifiers" );
            trait_modifiers.CheckConfiguration();

            CHECK_LN( false, lineNumber ); // should not get here
        }
        catch( DetailedException& re )
        {
            std::string msg = re.GetMsg();
            if( msg.find( rExpMsg ) == string::npos )
            {
                PrintDebug( rExpMsg + "\n" );
                PrintDebug( msg + "\n" );
                CHECK_LN( false, lineNumber );
            }
        }
    }

    TEST_FIXTURE( VectorTraitModifiersFixture, TestOocystAndSporozoiteTraitModifiers )
    {
        EnvPtr->Config = Environment::LoadConfigurationFile( "testdata/VectorTraitModifiersTest/TestOocystAndSporozoiteTraitModifiers.json" );
        try
        {
            ParasiteGenetics::CreateInstance()->Configure( Environment::CopyFromElement( (*EnvPtr->Config)[ "Parasite_Genetics" ]) );
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        VectorGeneCollection gene_collection;
        try
        {
            gene_collection.ConfigureFromJsonAndKey( EnvPtr->Config, "Genes" );
            gene_collection.CheckConfiguration();
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        VectorTraitModifiers trait_modifiers( &gene_collection );
        try
        {
            trait_modifiers.ConfigureFromJsonAndKey( EnvPtr->Config, "Gene_To_Trait_Modifiers" );
            trait_modifiers.CheckConfiguration();
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            CHECK( false );
        }

        CHECK_EQUAL( 2, gene_collection.Size() );
        CHECK_EQUAL( 2, trait_modifiers.Size() );

        VectorGenome vg_female_a1_a1; // VectorGenome that goes with ParasiteBarcode="AAA"
        vg_female_a1_a1.SetLocus( 0, 0, 0 ); // female
        vg_female_a1_a1.SetLocus( 1, 0, 0 ); // a1,a1
        CHECK_EQUAL( "X-a1:X-a1", gene_collection.GetGenomeName( vg_female_a1_a1 ) );

        VectorGenome vg_female_a1_a2; // VectorGenome not of interest
        vg_female_a1_a2.SetLocus( 0, 0, 0 ); // female
        vg_female_a1_a2.SetLocus( 1, 0, 1 ); // a1,a2
        CHECK_EQUAL( "X-a1:X-a2", gene_collection.GetGenomeName( vg_female_a1_a2 ) );

        VectorGenome vg_female_a2_a2; // VectorGenome that goes with ParasiteBarcode="CC*"
        vg_female_a2_a2.SetLocus( 0, 0, 0 ); // female
        vg_female_a2_a2.SetLocus( 1, 1, 1 ); // a2,a2
        CHECK_EQUAL( "X-a2:X-a2", gene_collection.GetGenomeName( vg_female_a2_a2 ) );

        std::vector<int64_t> hashes_aaa = ParasiteGenetics::GetInstance()->FindPossibleBarcodeHashcodes( "test", "AAA" );
        int64_t aaa = hashes_aaa[ 0 ]; // parasite barcode hash that goes with vg_female_a1_a1

        std::vector<int64_t> hashes_ggg = ParasiteGenetics::GetInstance()->FindPossibleBarcodeHashcodes( "test", "GGG" );
        int64_t ggg = hashes_ggg[ 0 ]; // parasite barcode hash that goes with vg_female_a1_a1

        std::vector<int64_t> hashes_ccc = ParasiteGenetics::GetInstance()->FindPossibleBarcodeHashcodes( "test", "CCC" );
        int64_t ccc = hashes_ccc[ 0 ]; // parasite barcode hash that goes with vg_female_a2_a2

        std::vector<int64_t> hashes_cca = ParasiteGenetics::GetInstance()->FindPossibleBarcodeHashcodes( "test", "CCA" );
        int64_t cca = hashes_cca[ 0 ]; // parasite barcode hash that goes with vg_female_a2_a2

        std::vector<int64_t> hashes_acc = ParasiteGenetics::GetInstance()->FindPossibleBarcodeHashcodes( "test", "ACC" );
        int64_t acc = hashes_acc[ 0 ]; // parasite barcode hash that does NOT go with either genome

        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::OOCYST_PROGRESSION, vg_female_a1_a1,   0, ggg ), FLT_EPSILON );
        CHECK_CLOSE( 2.0, trait_modifiers.GetModifier( VectorTrait::OOCYST_PROGRESSION, vg_female_a1_a1, aaa, ggg ), FLT_EPSILON );
        CHECK_CLOSE( 2.0, trait_modifiers.GetModifier( VectorTrait::OOCYST_PROGRESSION, vg_female_a1_a1, ggg, aaa ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::OOCYST_PROGRESSION, vg_female_a1_a1, aaa, cca ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::OOCYST_PROGRESSION, vg_female_a1_a1, cca, ggg ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::OOCYST_PROGRESSION, vg_female_a1_a1, ggg, cca ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::OOCYST_PROGRESSION, vg_female_a1_a1, cca, aaa ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::OOCYST_PROGRESSION, vg_female_a1_a2, aaa, ggg ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::OOCYST_PROGRESSION, vg_female_a2_a2, aaa, ggg ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::OOCYST_PROGRESSION, vg_female_a1_a1, ccc, ggg ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::OOCYST_PROGRESSION, vg_female_a1_a2, ccc, ggg ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::OOCYST_PROGRESSION, vg_female_a2_a2, ccc, ggg ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::OOCYST_PROGRESSION, vg_female_a2_a2, cca, ggg ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::OOCYST_PROGRESSION, vg_female_a2_a2, acc, ggg ), FLT_EPSILON );

        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::SPOROZOITE_MORTALITY, vg_female_a1_a1, 0   ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::SPOROZOITE_MORTALITY, vg_female_a1_a1, aaa ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::SPOROZOITE_MORTALITY, vg_female_a1_a2, aaa ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::SPOROZOITE_MORTALITY, vg_female_a2_a2, aaa ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::SPOROZOITE_MORTALITY, vg_female_a1_a1, ccc ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::SPOROZOITE_MORTALITY, vg_female_a1_a2, ccc ), FLT_EPSILON );
        CHECK_CLOSE( 2.0, trait_modifiers.GetModifier( VectorTrait::SPOROZOITE_MORTALITY, vg_female_a2_a2, ccc ), FLT_EPSILON );
        CHECK_CLOSE( 2.0, trait_modifiers.GetModifier( VectorTrait::SPOROZOITE_MORTALITY, vg_female_a2_a2, cca ), FLT_EPSILON );
        CHECK_CLOSE( 1.0, trait_modifiers.GetModifier( VectorTrait::SPOROZOITE_MORTALITY, vg_female_a2_a2, acc ), FLT_EPSILON );
    }

    TEST_FIXTURE( VectorTraitModifiersFixture, TestInvalidMalariaModel )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorTraitModifiersTest/TestInvalidMalariaModel.json",
                                       "'Trait' set to 'OOCYST_PROGRESSION' or 'SPOROZOITE_MORTALITY' can only be used with 'Malaria_Model'='MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS'.", true );
    }

    TEST_FIXTURE( VectorTraitModifiersFixture, TestInvalidBarcodeEmpty )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorTraitModifiersTest/TestInvalidBarcodeEmpty.json",
                                       "'Gametocyte_A_Barcode_String' in 'Trait_Modifiers' is empty.\nWhen 'Trait' = 'OOCYST_PROGRESSION',\nyou must define 'Gametocyte_A_Barcode_String' and it must have the same\nnumber of characters as the number of positions in the barcode.", true );
    }

    TEST_FIXTURE( VectorTraitModifiersFixture, TestInvalidBarcodeNotSpecified )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorTraitModifiersTest/TestInvalidBarcodeNotSpecified.json",
                                       "Parameter 'Sporozoite_Barcode_String of TraitModifier' not found in input file 'testdata/VectorTraitModifiersTest/TestInvalidBarcodeNotSpecified.json'.", true );
    }

    TEST_FIXTURE( VectorTraitModifiersFixture, TestInvalidBarcodeTooLong )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorTraitModifiersTest/TestInvalidBarcodeTooLong.json",
                                       "The 'Gametocyte_B_Barcode_String' = 'GGGGGG' is invalid.\nIt has 6 characters and 'Barcode_Genome_Locations' says you must have 3.", true );
    }

    TEST_FIXTURE( VectorTraitModifiersFixture, TestInvalidBarcodeBadCharacter )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorTraitModifiersTest/TestInvalidBarcodeBadCharacter.json",
                                       "The character 'X' in the parameter 'Sporozoite_Barcode_String' is invalid.\nValid values are: 'A', 'C', 'G', 'T'", true );
    }

    TEST_FIXTURE( VectorTraitModifiersFixture, TestNoAlleleCombinationPairs )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorTraitModifiersTest/TestNoAlleleCombinationPairs.json",
                                       "The parameter 'Allele_Combinations' has no combinations.\nYou must define at least one combination." );
    }

    TEST_FIXTURE( VectorTraitModifiersFixture, TestTraitInvalidTrait )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorTraitModifiersTest/TestTraitInvalidTrait.json",
                                       "Failed to find enum match for value 'BAD_TRAIT' and key 'Trait'.\nPossible values are:\nINFECTED_BY_HUMAN\nFECUNDITY\nFEMALE_EGG_RATIO\nSTERILITY\nTRANSMISSION_TO_HUMAN\nADJUST_FERTILE_EGGS\nMORTALITY\nINFECTED_PROGRESS" );
    }

    TEST_FIXTURE( VectorTraitModifiersFixture, TestTraitInvalidModifierLow )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorTraitModifiersTest/TestTraitInvalidModifierLow.json",
                                       "Configuration variable 'Modifier' with value -0.9 out of range: less than 0." );
    }

    TEST_FIXTURE( VectorTraitModifiersFixture, TestTraitInvalidModifierHigh )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorTraitModifiersTest/TestTraitInvalidModifierHigh2.json",
                                       "Configuration variable 'Modifier' with value 9999 out of range: greater than 1000." );
    }

    TEST_FIXTURE( VectorTraitModifiersFixture, TestTraitNoModifiers )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorTraitModifiersTest/TestTraitNoModifiers.json",
                                       "The 'Trait_Modifiers' for 'Gene_To_Trait_Modifier' cannot be empty." );
    }

    TEST_FIXTURE( VectorTraitModifiersFixture, TestAlleleCombinationTwoStars )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorTraitModifiersTest/TestAlleleCombinationTwoStars.json",
                                       "The parameter 'Allele_Combinations' has the allele pair #1 with\n'*' and '*'.  You can only use the '*' for one allele of the pair." );
    }
}
