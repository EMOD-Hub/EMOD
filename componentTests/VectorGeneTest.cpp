
#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"
#include "VectorGene.h"

#include <bitset>

using namespace Kernel;

SUITE( VectorGeneTest )
{
    struct VectorGeneFixture
    {
        VectorGeneFixture()
        {
            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = false;
            JsonConfigurable::_track_missing = false;
        }

        ~VectorGeneFixture()
        {
            JsonConfigurable::ClearMissingParameters();
            JsonConfigurable::_useDefaults = true;
            JsonConfigurable::_track_missing = true;
        }
    };

    TEST_FIXTURE( VectorGeneFixture, TestConfigureWithGender )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/VectorGeneTest/TestConfigureWithGender.json" ) );

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

        CHECK_EQUAL( 6, gene_collection.Size() );
        VectorGene* p_gene_x = gene_collection[ 0 ];
        VectorGene* p_gene_a = gene_collection[ 1 ];
        VectorGene* p_gene_b = gene_collection[ 2 ];
        VectorGene* p_gene_c = gene_collection[ 3 ];
        VectorGene* p_gene_d = gene_collection[ 4 ];
        VectorGene* p_gene_e = gene_collection[ 5 ];

        uint8_t z = 0; // had to do this because the compiler kept thinking uint8_t(0) was a character

        CHECK_EQUAL( 5, p_gene_x->GetNumAllele() );
        CHECK_EQUAL( std::string( "X" ),     p_gene_x->GetAllele( z )->GetName() );
        CHECK_EQUAL( (VectorAllele*)nullptr, p_gene_x->GetAllele( 1 )            );
        CHECK_EQUAL( (VectorAllele*)nullptr, p_gene_x->GetAllele( 2 )            );
        CHECK_EQUAL( (VectorAllele*)nullptr, p_gene_x->GetAllele( 3 )            );
        CHECK_EQUAL( std::string( "Y" ),     p_gene_x->GetAllele( 4 )->GetName() );
        CHECK_CLOSE( 0.6, p_gene_x->GetAllele( z )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.4, p_gene_x->GetAllele( 4 )->GetFrequency(), FLT_EPSILON );
        CHECK_EQUAL( 0, p_gene_x->GetAllele( z )->GetMutations().size() );
        CHECK_EQUAL( 0, p_gene_x->GetAllele( 4 )->GetMutations().size() );

        CHECK_EQUAL( 3, p_gene_a->GetNumAllele() );
        CHECK_EQUAL( std::string( "a1" ), p_gene_a->GetAllele( z )->GetName() );
        CHECK_EQUAL( std::string( "a2" ), p_gene_a->GetAllele( 1 )->GetName() );
        CHECK_EQUAL( std::string( "a3" ), p_gene_a->GetAllele( 2 )->GetName() );
        CHECK_CLOSE( 1.0, p_gene_a->GetAllele( z )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.0, p_gene_a->GetAllele( 1 )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.0, p_gene_a->GetAllele( 2 )->GetFrequency(), FLT_EPSILON );
        CHECK_EQUAL( 1, p_gene_a->GetAllele( z )->GetMutations().size() );
        CHECK_EQUAL( 0, p_gene_a->GetAllele( 1 )->GetMutations().size() );
        CHECK_EQUAL( 0, p_gene_a->GetAllele( 2 )->GetMutations().size() );

        CHECK_EQUAL( 6, p_gene_b->GetNumAllele() );
        CHECK_EQUAL( std::string( "b1" ), p_gene_b->GetAllele( z )->GetName() );
        CHECK_EQUAL( std::string( "b2" ), p_gene_b->GetAllele( 1 )->GetName() );
        CHECK_EQUAL( std::string( "b3" ), p_gene_b->GetAllele( 2 )->GetName() );
        CHECK_EQUAL( std::string( "b4" ), p_gene_b->GetAllele( 3 )->GetName() );
        CHECK_EQUAL( std::string( "b5" ), p_gene_b->GetAllele( 4 )->GetName() );
        CHECK_EQUAL( std::string( "b6" ), p_gene_b->GetAllele( 5 )->GetName() );
        CHECK_CLOSE( 0.4, p_gene_b->GetAllele( z )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.3, p_gene_b->GetAllele( 1 )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.2, p_gene_b->GetAllele( 2 )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.1, p_gene_b->GetAllele( 3 )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.0, p_gene_b->GetAllele( 4 )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.0, p_gene_b->GetAllele( 5 )->GetFrequency(), FLT_EPSILON );
        CHECK_EQUAL( 0, p_gene_b->GetAllele( z )->GetMutations().size() );
        CHECK_EQUAL( 2, p_gene_b->GetAllele( 1 )->GetMutations().size() );
        CHECK_EQUAL( 1, p_gene_b->GetAllele( 2 )->GetMutations().size() );
        CHECK_EQUAL( 0, p_gene_b->GetAllele( 3 )->GetMutations().size() );
        CHECK_EQUAL( 0, p_gene_b->GetAllele( 4 )->GetMutations().size() );
        CHECK_EQUAL( 0, p_gene_b->GetAllele( 5 )->GetMutations().size() );

        CHECK_EQUAL( 3, p_gene_b->GetAllele( 1 )->GetMutations()[ 0 ]->GetAlleleIndexTo() );
        CHECK_EQUAL( 4, p_gene_b->GetAllele( 1 )->GetMutations()[ 1 ]->GetAlleleIndexTo() );
        CHECK_CLOSE( 0.01, p_gene_b->GetAllele( 1 )->GetMutations()[ 0 ]->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.02, p_gene_b->GetAllele( 1 )->GetMutations()[ 1 ]->GetFrequency(), FLT_EPSILON );

        CHECK_EQUAL( 5, p_gene_b->GetAllele( 2 )->GetMutations()[ 0 ]->GetAlleleIndexTo() );
        CHECK_CLOSE( 0.03, p_gene_b->GetAllele( 2 )->GetMutations()[ 0 ]->GetFrequency(), FLT_EPSILON );

        CHECK_EQUAL( 3, p_gene_c->GetNumAllele() );
        CHECK_EQUAL( std::string( "c1" ), p_gene_c->GetAllele( z )->GetName() );
        CHECK_EQUAL( std::string( "c2" ), p_gene_c->GetAllele( 1 )->GetName() );
        CHECK_EQUAL( std::string( "c3" ), p_gene_c->GetAllele( 2 )->GetName() );
        CHECK_CLOSE( 0.5, p_gene_c->GetAllele( z )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.5, p_gene_c->GetAllele( 1 )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.0, p_gene_c->GetAllele( 2 )->GetFrequency(), FLT_EPSILON );
        CHECK_EQUAL( 0, p_gene_c->GetAllele( z )->GetMutations().size() );
        CHECK_EQUAL( 1, p_gene_c->GetAllele( 1 )->GetMutations().size() );
        CHECK_EQUAL( 0, p_gene_c->GetAllele( 2 )->GetMutations().size() );

        CHECK_EQUAL( 2, p_gene_d->GetNumAllele() );
        CHECK_EQUAL( std::string( "d1" ), p_gene_d->GetAllele( z )->GetName() );
        CHECK_EQUAL( std::string( "d2" ), p_gene_d->GetAllele( 1 )->GetName() );
        CHECK_CLOSE( 0.5, p_gene_d->GetAllele( z )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.5, p_gene_d->GetAllele( 1 )->GetFrequency(), FLT_EPSILON );
        CHECK_EQUAL( 0, p_gene_d->GetAllele( z )->GetMutations().size() );
        CHECK_EQUAL( 0, p_gene_d->GetAllele( 1 )->GetMutations().size() );

        CHECK_EQUAL( 2, p_gene_e->GetNumAllele() );
        CHECK_EQUAL( std::string( "e1" ), p_gene_e->GetAllele( z )->GetName() );
        CHECK_EQUAL( std::string( "e2" ), p_gene_e->GetAllele( 1 )->GetName() );
        CHECK_CLOSE( 0.5, p_gene_e->GetAllele( z )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.5, p_gene_e->GetAllele( 1 )->GetFrequency(), FLT_EPSILON );
        CHECK_EQUAL( 0, p_gene_e->GetAllele( z )->GetMutations().size() );
        CHECK_EQUAL( 0, p_gene_e->GetAllele( 1 )->GetMutations().size() );

        // -----------------------------
        // --- Test IsValidAlleleName()
        // -----------------------------
        CHECK(  gene_collection.IsValidAlleleName( "X" ) );
        CHECK(  gene_collection.IsValidAlleleName( "Y" ) );
        CHECK(  gene_collection.IsValidAlleleName( "b3" ) );
        CHECK( !gene_collection.IsValidAlleleName( "ZZZ" ) );
        CHECK( !gene_collection.IsValidAlleleName( "B3" ) );

        // -------------------------
        // --- Test GetLocusIndex()
        // -------------------------
        CHECK_EQUAL( 0, gene_collection.GetLocusIndex( "X" ) );
        CHECK_EQUAL( 0, gene_collection.GetLocusIndex( "Y" ) );
        CHECK_EQUAL( 1, gene_collection.GetLocusIndex( "a1" ) );
        CHECK_EQUAL( 2, gene_collection.GetLocusIndex( "b4" ) );
        CHECK_EQUAL( 3, gene_collection.GetLocusIndex( "c1" ) );
        CHECK_EQUAL( 4, gene_collection.GetLocusIndex( "d2" ) );
        CHECK_EQUAL( 5, gene_collection.GetLocusIndex( "e1" ) );

        // -------------------------
        // --- Test GetAlleleIndex()
        // -------------------------
        CHECK_EQUAL( 0, gene_collection.GetAlleleIndex( "X" ) );
        CHECK_EQUAL( 4, gene_collection.GetAlleleIndex( "Y" ) );
        CHECK_EQUAL( 0, gene_collection.GetAlleleIndex( "a1" ) );
        CHECK_EQUAL( 3, gene_collection.GetAlleleIndex( "b4" ) );
        CHECK_EQUAL( 0, gene_collection.GetAlleleIndex( "c1" ) );
        CHECK_EQUAL( 1, gene_collection.GetAlleleIndex( "d2" ) );
        CHECK_EQUAL( 0, gene_collection.GetAlleleIndex( "e1" ) );
    }

    TEST_FIXTURE( VectorGeneFixture, TestConfigureAutoAddGender )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/VectorGeneTest/TestConfigureAutoAddGender.json" ) );

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

        // 5 in file  but gender added to front.
        CHECK_EQUAL( 6, gene_collection.Size() );
        VectorGene* p_gene_x = gene_collection[ 0 ];
        VectorGene* p_gene_a = gene_collection[ 1 ];
        VectorGene* p_gene_b = gene_collection[ 2 ];
        VectorGene* p_gene_c = gene_collection[ 3 ];
        VectorGene* p_gene_d = gene_collection[ 4 ];
        VectorGene* p_gene_e = gene_collection[ 5 ];

        uint8_t z = 0; // had to do this because the compiler kept thinking uint8_t(0) was a character

        CHECK_EQUAL( 5, p_gene_x->GetNumAllele() );
        CHECK_EQUAL( std::string( "X" ),     p_gene_x->GetAllele( z )->GetName() );
        CHECK_EQUAL( (VectorAllele*)nullptr, p_gene_x->GetAllele( 1 )            );
        CHECK_EQUAL( (VectorAllele*)nullptr, p_gene_x->GetAllele( 2 )            );
        CHECK_EQUAL( (VectorAllele*)nullptr, p_gene_x->GetAllele( 3 )            );
        CHECK_EQUAL( std::string( "Y" ),     p_gene_x->GetAllele( 4 )->GetName() );
        CHECK_CLOSE( 0.75, p_gene_x->GetAllele( z )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.25, p_gene_x->GetAllele( 4 )->GetFrequency(), FLT_EPSILON );
        CHECK_EQUAL( 0, p_gene_x->GetAllele( z )->GetMutations().size() );
        CHECK_EQUAL( 0, p_gene_x->GetAllele( 4 )->GetMutations().size() );

        CHECK_EQUAL( 3, p_gene_a->GetNumAllele() );
        CHECK_EQUAL( std::string( "a1" ), p_gene_a->GetAllele( z )->GetName() );
        CHECK_EQUAL( std::string( "a2" ), p_gene_a->GetAllele( 1 )->GetName() );
        CHECK_EQUAL( std::string( "a3" ), p_gene_a->GetAllele( 2 )->GetName() );

        CHECK_EQUAL( 6, p_gene_b->GetNumAllele() );
        CHECK_EQUAL( 3, p_gene_c->GetNumAllele() );
        CHECK_EQUAL( 2, p_gene_d->GetNumAllele() );
        CHECK_EQUAL( 2, p_gene_e->GetNumAllele() );
    }

    TEST_FIXTURE( VectorGeneFixture, TestConfigureWithMultipleGenderAllelesA )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/VectorGeneTest/TestConfigureWithMultipleGenderAllelesA.json" ) );

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

        // 5 in file  but gender added to front.
        CHECK_EQUAL( 2, gene_collection.Size() );
        VectorGene* p_gene_x = gene_collection[ 0 ];
        VectorGene* p_gene_a = gene_collection[ 1 ];

        uint8_t z = 0; // had to do this because the compiler kept thinking uint8_t(0) was a character

        CHECK_EQUAL( 7, p_gene_x->GetNumAllele() );
        CHECK_EQUAL( std::string( "X"  ),    p_gene_x->GetAllele( z )->GetName() );
        CHECK_EQUAL( std::string( "X#" ),    p_gene_x->GetAllele( 1 )->GetName() );
        CHECK_EQUAL( (VectorAllele*)nullptr, p_gene_x->GetAllele( 2 )            );
        CHECK_EQUAL( (VectorAllele*)nullptr, p_gene_x->GetAllele( 3 )            );
        CHECK_EQUAL( std::string( "Y" ),     p_gene_x->GetAllele( 4 )->GetName() );
        CHECK_EQUAL( std::string( "Y1" ),    p_gene_x->GetAllele( 5 )->GetName() );
        CHECK_EQUAL( std::string( "Y2" ),    p_gene_x->GetAllele( 6 )->GetName() );
        CHECK_CLOSE( 0.40, p_gene_x->GetAllele( z )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.10, p_gene_x->GetAllele( 1 )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.30, p_gene_x->GetAllele( 4 )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.15, p_gene_x->GetAllele( 5 )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.05, p_gene_x->GetAllele( 6 )->GetFrequency(), FLT_EPSILON );
        CHECK_EQUAL( 0, p_gene_x->GetAllele( z )->GetMutations().size() );
        CHECK_EQUAL( 1, p_gene_x->GetAllele( 1 )->GetMutations().size() );
        CHECK_EQUAL( 2, p_gene_x->GetAllele( 4 )->GetMutations().size() );
        CHECK_EQUAL( 1, p_gene_x->GetAllele( 5 )->GetMutations().size() );
        CHECK_EQUAL( 0, p_gene_x->GetAllele( 6 )->GetMutations().size() );

        CHECK_EQUAL( 3, p_gene_a->GetNumAllele() );
        CHECK_EQUAL( std::string( "a1" ), p_gene_a->GetAllele( z )->GetName() );
        CHECK_EQUAL( std::string( "a2" ), p_gene_a->GetAllele( 1 )->GetName() );
        CHECK_EQUAL( std::string( "a3" ), p_gene_a->GetAllele( 2 )->GetName() );
    }

    TEST_FIXTURE( VectorGeneFixture, TestConfigureWithMultipleGenderAllelesB )
    {
        // ------------------------------------------------------------------------
        // --- Compared to test 'A', we move the X & Y so things are out of order
        // --- We want to see that they are put back in order.
        // ------------------------------------------------------------------------
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/VectorGeneTest/TestConfigureWithMultipleGenderAllelesB.json" ) );

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

        CHECK_EQUAL( 2, gene_collection.Size() );
        VectorGene* p_gene_x = gene_collection[ 0 ];
        VectorGene* p_gene_a = gene_collection[ 1 ];

        uint8_t z = 0; // had to do this because the compiler kept thinking uint8_t(0) was a character

        CHECK_EQUAL( 7, p_gene_x->GetNumAllele() );
        CHECK_EQUAL( std::string( "X"  ),    p_gene_x->GetAllele( z )->GetName() );
        CHECK_EQUAL( std::string( "X#" ),    p_gene_x->GetAllele( 1 )->GetName() );
        CHECK_EQUAL( (VectorAllele*)nullptr, p_gene_x->GetAllele( 2 )            );
        CHECK_EQUAL( (VectorAllele*)nullptr, p_gene_x->GetAllele( 3 )            );
        CHECK_EQUAL( std::string( "Y" ),     p_gene_x->GetAllele( 4 )->GetName() );
        CHECK_EQUAL( std::string( "Y1" ),    p_gene_x->GetAllele( 5 )->GetName() );
        CHECK_EQUAL( std::string( "Y2" ),    p_gene_x->GetAllele( 6 )->GetName() );
        CHECK_CLOSE( 0.40, p_gene_x->GetAllele( z )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.10, p_gene_x->GetAllele( 1 )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.30, p_gene_x->GetAllele( 4 )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.15, p_gene_x->GetAllele( 5 )->GetFrequency(), FLT_EPSILON );
        CHECK_CLOSE( 0.05, p_gene_x->GetAllele( 6 )->GetFrequency(), FLT_EPSILON );
        CHECK_EQUAL( 0, p_gene_x->GetAllele( z )->GetMutations().size() );
        CHECK_EQUAL( 1, p_gene_x->GetAllele( 1 )->GetMutations().size() );
        CHECK_EQUAL( 2, p_gene_x->GetAllele( 4 )->GetMutations().size() );
        CHECK_EQUAL( 1, p_gene_x->GetAllele( 5 )->GetMutations().size() );
        CHECK_EQUAL( 0, p_gene_x->GetAllele( 6 )->GetMutations().size() );

        CHECK_EQUAL( 3, p_gene_a->GetNumAllele() );
        CHECK_EQUAL( std::string( "a1" ), p_gene_a->GetAllele( z )->GetName() );
        CHECK_EQUAL( std::string( "a2" ), p_gene_a->GetAllele( 1 )->GetName() );
        CHECK_EQUAL( std::string( "a3" ), p_gene_a->GetAllele( 2 )->GetName() );
    }

    TEST_FIXTURE( VectorGeneFixture, TestGetGenomeName )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/VectorGeneTest/TestConfigureWithGender.json" ) );

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

        CHECK_EQUAL( 6, gene_collection.Size() );

        VectorGenome genome;
        genome.SetLocus( 0, 0, 0 ); //  X:X
        genome.SetLocus( 1, 0, 1 ); // a1:a2
        genome.SetLocus( 2, 3, 4 ); // b4:b5
        genome.SetLocus( 3, 2, 0 ); // c3:c1
        genome.SetLocus( 4, 0, 0 ); // d1:d1
        genome.SetLocus( 5, 1, 1 ); // e2:e2

        std::string name = gene_collection.GetGenomeName( genome );
        CHECK_EQUAL( "X-a1-b4-c3-d1-e2:X-a2-b5-c1-d1-e2", name );
    }

    void TestHelper_ConvertException( VectorGeneCollection* pGeneCollection,
                                      const std::vector<std::vector<std::string>>& rAlleleCombos,
                                      int lineNumber,
                                      const std::string& rExpMsg )
    {
        try
        {
            VectorGameteBitPair_t bit_mask = 0;
            std::vector<VectorGameteBitPair_t> possible_genomes;
            pGeneCollection->ConvertAlleleCombinationsStrings( "TestConvertAlleleCombinationsStrings",
                                                               rAlleleCombos,
                                                               &bit_mask,
                                                               &possible_genomes );

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

    TEST_FIXTURE( VectorGeneFixture, TestConvertAlleleCombinationsStrings )
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/VectorGeneTest/TestConvertAlleleCombinationsStrings.json" ) );

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

        CHECK_EQUAL( 6, gene_collection.Size() );
        VectorGene* p_gene_x = gene_collection[ 0 ];
        VectorGene* p_gene_a = gene_collection[ 1 ];
        VectorGene* p_gene_b = gene_collection[ 2 ];
        VectorGene* p_gene_c = gene_collection[ 3 ];
        VectorGene* p_gene_d = gene_collection[ 4 ];
        VectorGene* p_gene_e = gene_collection[ 5 ];

        uint8_t z = 0; // had to do this because the compiler kept thinking uint8_t(0) was a character

        CHECK_EQUAL( 5, p_gene_x->GetNumAllele() );
        CHECK_EQUAL( 3, p_gene_a->GetNumAllele() );
        CHECK_EQUAL( 6, p_gene_b->GetNumAllele() );
        CHECK_EQUAL( 3, p_gene_c->GetNumAllele() );
        CHECK_EQUAL( 2, p_gene_d->GetNumAllele() );
        CHECK_EQUAL( 2, p_gene_e->GetNumAllele() );

        // -------------------------------------------
        // --- Test using valid parameters
        // -------------------------------------------
        std::vector<std::vector<std::string>> alleles_list;
        alleles_list.push_back( { "b2", "b3" } );
        alleles_list.push_back( { "d2", "d1" } );

        VectorGameteBitPair_t bit_mask = 0;
        std::vector<VectorGameteBitPair_t> possible_genomes;
        gene_collection.ConvertAlleleCombinationsStrings( "TestConvertAlleleCombinationsStrings",
                                                          alleles_list,
                                                          &bit_mask,
                                                          &possible_genomes );

        // 00 000 000 000 000 000 111 000 111 000 000 00 000 000 000 000 000 111 000 111 000 000
        CHECK_EQUAL( 125069447688640, bit_mask );

        CHECK_EQUAL( 4, possible_genomes.size() );
        for( auto bits : possible_genomes )
        {
            std::bitset<64> bs = bits;
            printf("%s\n",bs.to_string().c_str());
        }

        // 00 000 000 000 000 000 000 000 010 000 000 00 000 000 000 000 000 001 000 001 000 000 - [b3,b2][d1,d2]
        CHECK_EQUAL( 549755818048, possible_genomes[ 0 ] );

        // 00 000 000 000 000 000 000 000 001 000 000 00 000 000 000 000 000 001 000 010 000 000 - [b2,b3][d1,d2]
        CHECK_EQUAL( 274877911168, possible_genomes[ 1 ] );

        // 00 000 000 000 000 000 001 000 010 000 000 00 000 000 000 000 000 000 000 001 000 000 - [b3,b2][d2,d1]
        CHECK_EQUAL( 18141941858368, possible_genomes[ 2 ] );

        // 00 000 000 000 000 000 001 000 001 000 000 00 000 000 000 000 000 000 000 010 000 000 - [b2,b3][d2,d1]
        CHECK_EQUAL( 17867063951488, possible_genomes[ 3 ] );

        // -------------------------------------------
        // --- Test special gender case
        // -------------------------------------------
        alleles_list.clear();
        alleles_list.push_back( { "Y", "X" } );

        bit_mask = 0;
        possible_genomes.clear();
        gene_collection.ConvertAlleleCombinationsStrings( "TestConvertAlleleCombinationsStrings",
                                                          alleles_list,
                                                          &bit_mask,
                                                          &possible_genomes );

        // 00 000 000 000 000 000 000 000 000 000 111 00 000 000 000 000 000 000 000 000 000 111
        CHECK_EQUAL( 30064771079, bit_mask );

        CHECK_EQUAL( 1, possible_genomes.size() );

        // 00 000 000 000 000 000 000 000 000 000 100 00 000 000 000 000 000 000 000 000 000 000 - [X,Y]
        CHECK_EQUAL( 17179869184, possible_genomes[ 0 ] );

        // -------------------------------------------
        // --- Test special gender case
        // -------------------------------------------
        alleles_list.clear();
        alleles_list.push_back( { "X", "Y" } );

        bit_mask = 0;
        possible_genomes.clear();
        gene_collection.ConvertAlleleCombinationsStrings( "TestConvertAlleleCombinationsStrings",
                                                          alleles_list,
                                                          &bit_mask,
                                                          &possible_genomes );

        // 00 000 000 000 000 000 000 000 000 000 111 00 000 000 000 000 000 000 000 000 000 111
        CHECK_EQUAL( 30064771079, bit_mask );

        CHECK_EQUAL( 1, possible_genomes.size() );

        // 00 000 000 000 000 000 000 000 000 000 100 00 000 000 000 000 000 000 000 000 000 000 - [X,Y]
        CHECK_EQUAL( 17179869184, possible_genomes[ 0 ] );

        // -------------------------------------------
        // --- Test special gender case
        // -------------------------------------------
        alleles_list.clear();
        alleles_list.push_back( { "Y", "*" } );

        bit_mask = 0;
        possible_genomes.clear();
        gene_collection.ConvertAlleleCombinationsStrings( "TestConvertAlleleCombinationsStrings",
                                                          alleles_list,
                                                          &bit_mask,
                                                          &possible_genomes );

        // 00 000 000 000 000 000 000 000 000 000 111 00 000 000 000 000 000 000 000 000 000 111
        CHECK_EQUAL( 30064771079, bit_mask );

        CHECK_EQUAL( 1, possible_genomes.size() );

        // 00 000 000 000 000 000 000 000 000 000 100 00 000 000 000 000 000 000 000 000 000 000 - [X,Y]
        CHECK_EQUAL( 17179869184, possible_genomes[ 0 ] );

        // -------------------------------------------
        // --- Test special gender case
        // -------------------------------------------
        alleles_list.clear();
        alleles_list.push_back( { "X", "X" } );

        bit_mask = 0;
        possible_genomes.clear();
        gene_collection.ConvertAlleleCombinationsStrings( "TestConvertAlleleCombinationsStrings",
                                                          alleles_list,
                                                          &bit_mask,
                                                          &possible_genomes );

        // 00 000 000 000 000 000 000 000 000 000 111 00 000 000 000 000 000 000 000 000 000 111
        CHECK_EQUAL( 30064771079, bit_mask );

        CHECK_EQUAL( 1, possible_genomes.size() );

        // 00 000 000 000 000 000 000 000 000 000 000 00 000 000 000 000 000 000 000 000 000 000 - [X,Y]
        CHECK_EQUAL( 0, possible_genomes[ 0 ] );

        // -------------------------------------------
        // --- Test special gender case
        // -------------------------------------------
        alleles_list.clear();
        alleles_list.push_back( { "X", "*" } );

        bit_mask = 0;
        possible_genomes.clear();
        gene_collection.ConvertAlleleCombinationsStrings( "TestConvertAlleleCombinationsStrings",
                                                          alleles_list,
                                                          &bit_mask,
                                                          &possible_genomes );

        // 00 000 000 000 000 000 000 000 000 000 111 00 000 000 000 000 000 000 000 000 000 111
        CHECK_EQUAL( 30064771079, bit_mask );

        CHECK_EQUAL( 2, possible_genomes.size() );

        // 00 000 000 000 000 000 000 000 000 000 000 00 000 000 000 000 000 000 000 000 000 000 - [X,X]
        CHECK_EQUAL( 0, possible_genomes[ 0 ] );
        // 00 000 000 000 000 000 000 000 000 000 100 00 000 000 000 000 000 000 000 000 000 000 - [X,Y]
        CHECK_EQUAL( 17179869184, possible_genomes[ 1 ] );

        // -------------------------------------------
        // --- Test special gender case
        // -------------------------------------------
        alleles_list.clear();
        alleles_list.push_back( { "*", "X" } );

        bit_mask = 0;
        possible_genomes.clear();
        gene_collection.ConvertAlleleCombinationsStrings( "TestConvertAlleleCombinationsStrings",
                                                          alleles_list,
                                                          &bit_mask,
                                                          &possible_genomes );

        // 00 000 000 000 000 000 000 000 000 000 111 00 000 000 000 000 000 000 000 000 000 111
        CHECK_EQUAL( 30064771079, bit_mask );

        CHECK_EQUAL( 2, possible_genomes.size() );

        // 00 000 000 000 000 000 000 000 000 000 000 00 000 000 000 000 000 000 000 000 000 000 - [X,X]
        CHECK_EQUAL( 0, possible_genomes[ 0 ] );
        // 00 000 000 000 000 000 000 000 000 000 100 00 000 000 000 000 000 000 000 000 000 000 - [X,Y]
        CHECK_EQUAL( 17179869184, possible_genomes[ 1 ] );

        // ------------------
        // --- Invalid gender case
        // ------------------
        alleles_list.clear();
        TestHelper_ConvertException( &gene_collection, alleles_list, __LINE__,
                                     "The parameter 'TestConvertAlleleCombinationsStrings' has no combinations.\nYou must define at least one combination." );

        // ------------------
        // --- No pairs defined
        // ------------------
        alleles_list.clear();
        alleles_list.push_back( { "Y", "Y" } );
        TestHelper_ConvertException( &gene_collection, alleles_list, __LINE__,
                                     "The parameter 'TestConvertAlleleCombinationsStrings' has the allele pair #1 with\nallele 'Y' and 'Y'.  The gender locus must have one 'X'." );

        // ------------------
        // --- Too many pairs
        // ------------------
        alleles_list.clear();
        alleles_list.push_back( { "X", "Y" } );
        alleles_list.push_back( { "a1", "a2" } );
        alleles_list.push_back( { "b2", "b3" } );
        alleles_list.push_back( { "c2", "c1" } );
        alleles_list.push_back( { "d2", "d1" } );
        alleles_list.push_back( { "e1", "e1" } );
        alleles_list.push_back( { "f2", "f2" } ); // invalid test

        std::string exp = "The parameter 'TestConvertAlleleCombinationsStrings' has too many combinations.\n";
        exp += "'TestConvertAlleleCombinationsStrings' = [['X','Y'],['a1','a2'],['b2','b3'],['c2','c1'],['d2','d1'],['e1','e1'],['f2','f2']]\n";
        exp += "The following genes/loci and their alleles are defined:\n";
        exp += "['X','Y']\n";
        exp += "['a1','a2','a3']\n";
        exp += "['b1','b2','b3','b4','b5','b6']\n";
        exp += "['c1','c2','c3']\n";
        exp += "['d1','d2']\n";
        exp += "['e1','e2']\n";
        exp += "'TestConvertAlleleCombinationsStrings' can define at most one allele pair per locus.";
        TestHelper_ConvertException( &gene_collection, alleles_list, __LINE__, exp );

        // --------------------
        // --- Entry not a pair
        // --------------------
        alleles_list.clear();
        alleles_list.push_back( { "X", "Y" } );
        alleles_list.push_back( { "a1", "a2", "a3" } ); // third entry
        TestHelper_ConvertException( &gene_collection, alleles_list, __LINE__,
                                     "The parameter 'TestConvertAlleleCombinationsStrings' has the allele pair #2 with\n3 elements instead of two." );

        // -----------------------------------------------
        // --- Allele in pair are not from the same gene
        // -----------------------------------------------
        alleles_list.clear();
        alleles_list.push_back( { "X", "Y" } );
        alleles_list.push_back( { "a1", "b2" } ); // a1 & b1 are not from the same gene
        TestHelper_ConvertException( &gene_collection, alleles_list, __LINE__,
                                     "The parameter 'TestConvertAlleleCombinationsStrings' has the allele pair #2 with\nallele 'a1' and 'b2' and they are not from the same Gene." );

        // -------------------------------------
        // --- Multiple pairs for the same gene
        // -------------------------------------
        alleles_list.clear();
        alleles_list.push_back( { "X", "Y" } );
        alleles_list.push_back( { "a1", "a2" } );
        alleles_list.push_back( { "b2", "b3" } ); // from the same 'b' gene
        alleles_list.push_back( { "c2", "c1" } );
        alleles_list.push_back( { "b4", "b5" } ); // from the same 'b' gene
        TestHelper_ConvertException( &gene_collection, alleles_list, __LINE__,
                                     "The parameter 'TestConvertAlleleCombinationsStrings' has the allele pair #5 with\nalleles ['b4','b5'] from a Gene already defined." );

        // -----------------------------------------------
        // --- Invalid allele name
        // -----------------------------------------------
        alleles_list.clear();
        alleles_list.push_back( { "X", "BAD" } ); // BAD is unknown
        TestHelper_ConvertException( &gene_collection, alleles_list, __LINE__,
                                     "The parameter 'TestConvertAlleleCombinationsStrings' has the allele pair #1 with\n'BAD' that is not an allele in one of the 'Genes'." );
    }

    void TestHelper_ConfigureException( int lineNumber, const std::string& rFilename, const std::string& rExpMsg )
    {
        try
        {
            unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( rFilename ) );

            VectorGeneCollection gene_collection;
            gene_collection.ConfigureFromJsonAndKey( p_config.get(), "Genes" );
            gene_collection.CheckConfiguration();

            CHECK_LN( false, lineNumber ); // should not get here
        }
        catch( DetailedException& re )
        {
            std::string msg = re.GetMsg();
            if( msg.find( rExpMsg ) == string::npos )
            {
                PrintDebug( rExpMsg+"\n" );
                PrintDebug( msg + "\n" );
                CHECK_LN( false, lineNumber );
            }
        }
    }

    TEST_FIXTURE( VectorGeneFixture, TestAlleleNoAllele )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestAlleleNoAllele.json",
                                       "The parameter 'Genes[x].Alleles' cannot have zero entries." );
    }

    TEST_FIXTURE( VectorGeneFixture, TestAlleleTooManyAllele )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestAlleleTooManyAllele.json",
                                       "The gene/locus has too many alleles defined (9).\nThere is a maximum of 8 allele per gene.\nThe alleles defined are:\nb1\nb2\nb3\nb4\nb5\nb6\nb7\nb8\nBAD" );
    }

    TEST_FIXTURE( VectorGeneFixture, TestAlleleDuplicate )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestAlleleDuplicate.json",
                                       "There is more than one allele with the name 'SAME'.\nAllele names must be unique." );
    }

    TEST_FIXTURE( VectorGeneFixture, TestAlleleFrequencyOutOfRangeHigh )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestAlleleFrequencyOutOfRangeHigh.json",
                                       "Configuration variable 'Initial_Allele_Frequency' with value 6.6 out of range: greater than 1." );
    }

    TEST_FIXTURE( VectorGeneFixture, TestAlleleFrequencyOutOfRangeLow )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestAlleleFrequencyOutOfRangeLow.json",
                                       "Configuration variable 'Initial_Allele_Frequency' with value -1 out of range: less than 0." );
    }

    TEST_FIXTURE( VectorGeneFixture, TestAlleleFrequencySumNotOne )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestAlleleFrequencySumNotOne.json",
                                       "Genes[x].Alleles frequencies do not sum to one.  The frequencies are:\n'a1' = 0.4\n'a2' = 0.3\nTotal Frequency = 0.7" );
    }

    TEST_FIXTURE( VectorGeneFixture, TestAlleleMutationFrequencyOutOfRangeHigh )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestAlleleMutationFrequencyOutOfRangeHigh.json",
                                       "Configuration variable 'Probability_Of_Mutation' with value 1.01 out of range: greater than 1." );
    }

    TEST_FIXTURE( VectorGeneFixture, TestAlleleMutationFrequencyOutOfRangeLow )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestAlleleMutationFrequencyOutOfRangeLow.json",
                                       "Configuration variable 'Probability_Of_Mutation' with value -1 out of range: less than 0." );
    }

    TEST_FIXTURE( VectorGeneFixture, TestAlleleMutationUnknownFromAllele )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestAlleleMutationUnknownFromAllele.json",
                                       "Constrained String (Mutate_From) with specified value 'XX' invalid. Possible values are: \na1\na2" );
    }

    TEST_FIXTURE( VectorGeneFixture, TestAlleleMutationUnknownToAllele )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestAlleleMutationUnknownToAllele.json",
                                       "Constrained String (Mutate_To) with specified value 'XX' invalid. Possible values are: \na1\na2" );
    }

    TEST_FIXTURE( VectorGeneFixture, TestAlleleMutationToSelf )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestAlleleMutationToSelf.json",
                                       "'Mutate_From' cannot equal 'Mutate_To'(=a1).\nNothing will change for an allele that mutates to itself." );
    }

    TEST_FIXTURE( VectorGeneFixture, TestGenderGeneOutOfOrder )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestGenderGeneOutOfOrder.json",
                                       "Allele names 'X' and 'Y' are only allowed for the Gender 'gene'.\nIf you are including the gender 'gene', then it must be the first in the list and both alleles defined." );
    }

    TEST_FIXTURE( VectorGeneFixture, TestGenderGeneMissingX )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestGenderGeneMissingX.json",
            "'Is_Gender_Gene' is set to true but less than 2 alleles are defined.\nFor gender genes, at least one male ('Is_Y_Chromosome':1) and one female ('Is_Y_Chromosome':0) allele must be defined." );
    }

    TEST_FIXTURE( VectorGeneFixture, TestGenderGeneMissingY )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestGenderGeneMissingY.json",
            "'Is_Gender_Gene' is set to true but less than 2 alleles are defined.\nFor gender genes, at least one male ('Is_Y_Chromosome':1) and one female ('Is_Y_Chromosome':0) allele must be defined." );
    }

    TEST_FIXTURE( VectorGeneFixture, TestTooManyGenes )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestTooManyGenes.json",
                                       "11 vector genes have been defined and the maximum is 8 when not defining gender gene and 9 when gender gene is included." );
    }

    TEST_FIXTURE( VectorGeneFixture, TestTooManyGenderAllelesA )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestTooManyGenderAllelesA.json",
                                       "Invalid number of alleles for one gender.\n6 female alleles were defined.  There can only be at most 4 of each gender." );
    }

    TEST_FIXTURE( VectorGeneFixture, TestTooManyGenderAllelesB )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestTooManyGenderAllelesB.json",
                                       "Invalid number of alleles for one gender.\n7 male alleles were defined.  There can only be at most 4 of each gender." );
    }

    TEST_FIXTURE( VectorGeneFixture, TestGenderGeneMaleFrequencyTooHigh )
    {
        TestHelper_ConfigureException( __LINE__, "testdata/VectorGeneTest/TestGenderGeneMaleFrequencyTooHigh.json",
                                       "Initial allele frequencies for male (Y-chromosome) alleles sum to" );
    }
}
