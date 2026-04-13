
#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"

#include "MicrosporidiaParameters.h"
#include "NodeVectorEventContext.h"
#include "VectorEnums.h"
#include "EventTrigger.h"

using namespace Kernel;

// Helper that constructs NodeVectorEventContextHost without a real Node.
// The destructor does not dereference the pointer, so nullptr is safe for
// tests that only exercise the microsporidia accumulation methods.
struct NodeVectorEventContextHostForTest : public NodeVectorEventContextHost
{
    NodeVectorEventContextHostForTest() : NodeVectorEventContextHost(static_cast<Node*>(nullptr)) {}
};

SUITE(MicrosporidiaTest)
{
    // -------------------------------------------------------------------------
    // --- MicrosporidiaCollection configuration tests
    // -------------------------------------------------------------------------

    struct MicrosporidiaFixture
    {
        MicrosporidiaFixture()
        {
            Environment::Finalize();
            Environment::setLogger(new SimpleLogger(Logger::tLevel::WARNING));
            JsonConfigurable::ClearMissingParameters();
        }

        ~MicrosporidiaFixture()
        {
            Environment::Finalize();
            JsonConfigurable::ClearMissingParameters();
        }

        void TestHelper_ConfigureException(int lineNumber,
                                           const std::string& filename,
                                           const std::string& expectedMsg)
        {
            MicrosporidiaCollection collection;
            try
            {
                unique_ptr<Configuration> p_config(Configuration_Load(filename));
                collection.ConfigureFromJsonAndKey(p_config.get(), "Microsporidia");
                collection.CheckConfiguration();
                CHECK_LN(false, lineNumber); // should not reach here
            }
            catch(DetailedException& re)
            {
                std::string msg = re.GetMsg();
                if(msg.find(expectedMsg) == string::npos)
                {
                    PrintDebug(expectedMsg);
                    PrintDebug(msg);
                    CHECK_LN(false, lineNumber);
                }
            }
        }
    };

    TEST_FIXTURE(MicrosporidiaFixture, TestConfigureSingleStrain)
    {
        unique_ptr<Configuration> p_config(
            Configuration_Load("testdata/MicrosporidiaTest/TestSingleStrain.json"));

        MicrosporidiaCollection collection;
        collection.ConfigureFromJsonAndKey(p_config.get(), "Microsporidia");
        collection.CheckConfiguration();

        // m_Collection always starts with the "NoMicrosporidia" entry at index 0
        CHECK_EQUAL(2, (int)collection.Size());

        const MicrosporidiaParameters& strain = collection.GetStrain("Strain_C");
        CHECK_EQUAL(1, strain.index);
        CHECK_EQUAL(std::string("Strain_C"), strain.strain_name);
        CHECK_CLOSE(0.0f, strain.female_to_egg_transmission_probability, FLT_EPSILON);
        CHECK_CLOSE(0.0f, strain.male_to_egg_transmission_probability,   FLT_EPSILON);
        CHECK_CLOSE(1.0f, strain.female_to_male_transmission_probability, FLT_EPSILON);
        CHECK_CLOSE(0.4f, strain.male_to_female_transmission_probability, FLT_EPSILON);
        CHECK_CLOSE(0.8f, strain.larval_growth_modifier,    FLT_EPSILON);
        CHECK_CLOSE(1.5f, strain.female_mortality_modifier, FLT_EPSILON);
        CHECK_CLOSE(1.2f, strain.male_mortality_modifier,   FLT_EPSILON);

        // Single-point modifier at t=0 → constant value across all durations
        CHECK_CLOSE(1.0f, strain.disease_acquisition_modifier.getValueLinearInterpolation(0.0f), FLT_EPSILON);
        CHECK_CLOSE(1.0f, strain.disease_transmission_modifier.getValueLinearInterpolation(0.0f), FLT_EPSILON);
    }

    TEST_FIXTURE(MicrosporidiaFixture, TestConfigureTwoStrains)
    {
        unique_ptr<Configuration> p_config(
            Configuration_Load("testdata/MicrosporidiaTest/TestTwoStrains.json"));

        MicrosporidiaCollection collection;
        collection.ConfigureFromJsonAndKey(p_config.get(), "Microsporidia");
        collection.CheckConfiguration();

        CHECK_EQUAL(3, (int)collection.Size());

        const MicrosporidiaParameters& strainC = collection.GetStrain("Strain_C");
        CHECK_EQUAL(1, strainC.index);
        CHECK_CLOSE(0.1f, strainC.female_mortality_modifier, FLT_EPSILON);
        CHECK_CLOSE(1.0f, strainC.disease_acquisition_modifier.getValueLinearInterpolation(0.0f), FLT_EPSILON);
        CHECK_CLOSE(1.0f, strainC.disease_transmission_modifier.getValueLinearInterpolation(0.0f), FLT_EPSILON);

        const MicrosporidiaParameters& strainD = collection.GetStrain("Strain_D");
        CHECK_EQUAL(2, strainD.index);
        CHECK_CLOSE(1.0f, strainD.female_mortality_modifier, FLT_EPSILON);
        CHECK_CLOSE(0.0f, strainD.disease_acquisition_modifier.getValueLinearInterpolation(0.0f), FLT_EPSILON);
        CHECK_CLOSE(0.0f, strainD.disease_transmission_modifier.getValueLinearInterpolation(0.0f), FLT_EPSILON);
    }

    TEST_FIXTURE(MicrosporidiaFixture, TestDiseaseModifiers)
    {
        unique_ptr<Configuration> p_config(
            Configuration_Load("testdata/MicrosporidiaTest/TestDiseaseModifiers.json"));

        MicrosporidiaCollection collection;
        collection.ConfigureFromJsonAndKey(p_config.get(), "Microsporidia");
        collection.CheckConfiguration();

        const MicrosporidiaParameters& strain = collection.GetStrain("Strain_E");

        // Acquisition modifier: 1.0 at t=0, linearly decreases to 0.0 at t=10
        CHECK_CLOSE(1.0f, strain.disease_acquisition_modifier.getValueLinearInterpolation(0.0f),  FLT_EPSILON);
        CHECK_CLOSE(0.5f, strain.disease_acquisition_modifier.getValueLinearInterpolation(5.0f),  0.001f);
        CHECK_CLOSE(0.0f, strain.disease_acquisition_modifier.getValueLinearInterpolation(10.0f), FLT_EPSILON);

        // Transmission modifier: 0.0 at t=0, linearly increases to 1.0 at t=10
        CHECK_CLOSE(0.0f, strain.disease_transmission_modifier.getValueLinearInterpolation(0.0f),  FLT_EPSILON);
        CHECK_CLOSE(0.5f, strain.disease_transmission_modifier.getValueLinearInterpolation(5.0f),  0.001f);
        CHECK_CLOSE(1.0f, strain.disease_transmission_modifier.getValueLinearInterpolation(10.0f), FLT_EPSILON);
    }

    TEST_FIXTURE(MicrosporidiaFixture, TestDefaultNoMicrosporidiaEntry)
    {
        unique_ptr<Configuration> p_config(
            Configuration_Load("testdata/MicrosporidiaTest/TestNoStrains.json"));

        MicrosporidiaCollection collection;
        collection.ConfigureFromJsonAndKey(p_config.get(), "Microsporidia");
        collection.CheckConfiguration();

        // Only the built-in "NoMicrosporidia" sentinel at index 0
        CHECK_EQUAL(1, (int)collection.Size());

        const MicrosporidiaParameters& no_strain = collection.GetStrain("NoMicrosporidia");
        CHECK_EQUAL(0, no_strain.index);
    }

    TEST_FIXTURE(MicrosporidiaFixture, TestMortalityModifierLists)
    {
        unique_ptr<Configuration> p_config(
            Configuration_Load("testdata/MicrosporidiaTest/TestTwoStrains.json"));

        MicrosporidiaCollection collection;
        collection.ConfigureFromJsonAndKey(p_config.get(), "Microsporidia");
        collection.CheckConfiguration();

        const std::vector<float>& female_list = collection.GetMortalityModifierListFemale();
        const std::vector<float>& male_list   = collection.GetMortalityModifierListMale();

        // index 0 = NoMicrosporidia (default modifier 1.0), 1 = Strain_C, 2 = Strain_D
        CHECK_EQUAL(3, (int)female_list.size());
        CHECK_EQUAL(3, (int)male_list.size());

        CHECK_CLOSE(1.0f, female_list[0], FLT_EPSILON); // NoMicrosporidia
        CHECK_CLOSE(0.1f, female_list[1], FLT_EPSILON); // Strain_C 
        CHECK_CLOSE(1.0f, female_list[2], FLT_EPSILON); // Strain_D

        CHECK_CLOSE(1.0f, male_list[0], FLT_EPSILON);
        CHECK_CLOSE(0.1f, male_list[1], FLT_EPSILON);
        CHECK_CLOSE(1.0f, male_list[2], FLT_EPSILON);
    }

    TEST_FIXTURE(MicrosporidiaFixture, TestGetStrainUnknownName)
    {
        unique_ptr<Configuration> p_config(
            Configuration_Load("testdata/MicrosporidiaTest/TestSingleStrain.json"));

        MicrosporidiaCollection collection;
        collection.ConfigureFromJsonAndKey(p_config.get(), "Microsporidia");
        collection.CheckConfiguration();

        bool threw = false;
        try
        {
            collection.GetStrain("UnknownStrain");
        }
        catch(DetailedException&)
        {
            threw = true;
        }
        CHECK(threw);
    }

    TEST_FIXTURE(MicrosporidiaFixture, TestEmptyStrainName)
    {
        TestHelper_ConfigureException(__LINE__,
            "testdata/MicrosporidiaTest/TestEmptyStrainName.json",
            "cannot be empty");
    }

    TEST_FIXTURE(MicrosporidiaFixture, TestReservedStrainName)
    {
        TestHelper_ConfigureException(__LINE__,
            "testdata/MicrosporidiaTest/TestReservedStrainName.json",
            "NoMicrosporidia");
    }

    TEST_FIXTURE(MicrosporidiaFixture, TestDuplicateStrainName)
    {
        TestHelper_ConfigureException(__LINE__,
            "testdata/MicrosporidiaTest/TestDuplicateStrainName.json",
            "Duplicate microsporidia");
    }

    TEST_FIXTURE(MicrosporidiaFixture, TestTooManyStrains)
    {
        TestHelper_ConfigureException(__LINE__,
            "testdata/MicrosporidiaTest/TestTooManyStrains.json",
            "Please reduce the number of strains you have to the maximum of 3");
    }

    // -------------------------------------------------------------------------
    // --- GetLarvalMicrosporidiaInfectivity accumulation tests
    // -------------------------------------------------------------------------

    // NodeVectorEventContextHost contains an IndividualEventBroadcaster member whose
    // constructor calls EventTriggerFactory::GetInstance()->GetNumEventTriggers().
    // m_pNEC is heap-allocated so it is constructed after the factory is initialized.
    struct InfectivityFixture
    {
        NodeVectorEventContextHostForTest* m_pNEC;

        InfectivityFixture() : m_pNEC(nullptr)
        {
            Environment::Finalize();
            Environment::setLogger(new SimpleLogger(Logger::tLevel::WARNING));

            json::Object fakeConfigJson;
            Configuration* fakeConfig = Environment::CopyFromElement(fakeConfigJson);
            EventTriggerFactory::GetInstance()->Configure(fakeConfig);

            m_pNEC = new NodeVectorEventContextHostForTest();
        }

        ~InfectivityFixture()
        {
            delete m_pNEC;
            EventTriggerFactory::DeleteInstance();
            Environment::Finalize();
        }
    };

    TEST_FIXTURE(InfectivityFixture, TestInfectivity_Empty)
    {
        auto result = m_pNEC->GetLarvalMicrosporidiaInfectivity(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A");

        CHECK_EQUAL(true, result.empty());
    }

    TEST_FIXTURE(InfectivityFixture, TestInfectivity_SingleIntervention)
    {
        // fraction = coverage * effect = 0.5 * 0.8 = 0.4
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A", 1, 0.5f, 0.8f);

        auto result = m_pNEC->GetLarvalMicrosporidiaInfectivity(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A");

        CHECK_EQUAL(MAX_MICROSPORIDIA_STRAINS, (int)result.size());
        CHECK_CLOSE(0.0f, result[0], FLT_EPSILON);
        CHECK_CLOSE(0.4f, result[1], FLT_EPSILON);
    }

    TEST_FIXTURE(InfectivityFixture, TestInfectivity_TwoInterventionsSameStrain)
    {
        // First:  infects 0.5 of all larvae          → strain 1 gets 0.50
        // Second: draws from remaining 0.5, infects half → strain 1 adds 0.25
        // Total for strain 1: 0.75
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A", 1, 0.5f, 1.0f);
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A", 1, 0.5f, 1.0f);

        auto result = m_pNEC->GetLarvalMicrosporidiaInfectivity(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A");

        CHECK_EQUAL(MAX_MICROSPORIDIA_STRAINS, (int)result.size());
        CHECK_CLOSE(0.0f,  result[0], FLT_EPSILON);
        CHECK_CLOSE(0.75f, result[1], 0.001f);
    }

    TEST_FIXTURE(InfectivityFixture, TestInfectivity_TwoInterventionsDifferentStrains)
    {
        // IV1: strain 1, cov=0.4, eff=1.0 → temp[1] = {0.4, 1.0}
        // IV2: strain 2, cov=0.5, eff=1.0
        //   overlap with strain 1: 0.5*0.4 split 50/50 by equal effects → each gets 0.1
        //   strain 1 keeps (1-0.5)*0.4 + 0.1 = 0.3
        //   strain 2 gets  0.1 from overlap + 0.6*0.5 from uninfected = 0.4
        // Final: strain 1 cov = 0.3, strain 2 cov = 0.4
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A", 1, 0.4f, 1.0f);
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A", 2, 0.5f, 1.0f);

        auto result = m_pNEC->GetLarvalMicrosporidiaInfectivity(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A");

        CHECK_EQUAL(MAX_MICROSPORIDIA_STRAINS, (int)result.size());
        CHECK_CLOSE(0.0f,  result[0], FLT_EPSILON);
        CHECK_CLOSE(0.30f, result[1], 0.001f);
        CHECK_CLOSE(0.40f, result[2], 0.001f);
    }

    TEST_FIXTURE(InfectivityFixture, TestInfectivity_ThreeStrains)
    {
        // IV1: strain 1, cov=0.3, eff=1.0 → temp[1] = {0.3, 1.0}
        // IV2: strain 2, cov=0.4, eff=1.0
        //   overlap with strain 1: 0.4*0.3/2 = 0.06 to each
        //   strain 1 → (1-0.4)*0.3 + 0.06 = 0.24
        //   strain 2 → 0.06 + 0.7*0.4 = 0.34
        // IV3: strain 3, cov=0.5, eff=1.0
        //   overlap with strain 1 (0.24): 0.5*0.24/2 = 0.06 each → strain 1 → 0.18
        //   overlap with strain 2 (0.34): 0.5*0.34/2 = 0.085 each → strain 2 → 0.255
        //   strain 3 → 0.06 + 0.085 + 0.42*0.5 = 0.355
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A", 1, 0.3f, 1.0f);
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A", 2, 0.4f, 1.0f);
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A", 3, 0.5f, 1.0f);

        auto result = m_pNEC->GetLarvalMicrosporidiaInfectivity(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A");

        CHECK_EQUAL(MAX_MICROSPORIDIA_STRAINS, (int)result.size());
        CHECK_CLOSE(0.0f,   result[0], FLT_EPSILON);
        CHECK_CLOSE(0.180f, result[1], 0.001f);
        CHECK_CLOSE(0.255f, result[2], 0.001f);
        CHECK_CLOSE(0.355f, result[3], 0.001f);
    }

    TEST_FIXTURE(InfectivityFixture, TestInfectivity_ThreeInterventionsTwoStrains)
    {
        // IV1: strain 1, cov=0.4, eff=1.0 → temp[1] = {0.4, 1.0}
        // IV2: strain 2, cov=0.5, eff=1.0
        //   strain 1 → 0.3, strain 2 → 0.4 (same as TwoDifferentStrains test)
        // IV3: strain 1, cov=0.5, eff=1.0
        //   overlap with strain 1 (0.3): 0.5*0.3/2 = 0.075 each → strain 1 → 0.225
        //   overlap with strain 2 (0.4): 0.5*0.4/2 = 0.1 each → strain 2 → 0.3
        //   iv3 gets 0.075 + 0.1 + 0.3*0.5 = 0.325
        //   merge iv3 into strain 1: coverage = 0.225 + 0.325 = 0.55, effect = 1.0
        // Final: strain 1 = 0.55, strain 2 = 0.30
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A", 1, 0.4f, 1.0f);
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A", 2, 0.5f, 1.0f);
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A", 1, 0.5f, 1.0f);

        auto result = m_pNEC->GetLarvalMicrosporidiaInfectivity(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A");

        CHECK_EQUAL(MAX_MICROSPORIDIA_STRAINS, (int)result.size());
        CHECK_CLOSE(0.0f,  result[0], FLT_EPSILON);
        CHECK_CLOSE(0.55f, result[1], 0.001f);
        CHECK_CLOSE(0.30f, result[2], 0.001f);
    }

    TEST_FIXTURE(InfectivityFixture, TestInfectivity_ThreeInterventionsTwoStrainsDifferentOrder)
    {
        // IV1: strain 1, cov=0.4, eff=1.0 → temp[1] = {0.4, 1.0}
        // IV2: strain 1, cov=0.5, eff=1.0
        //   overlap with strain 1 (0.4): 0.5*0.4/2 = 0.1 each → strain 1 → 0.3
        //   iv2 gets 0.1 from overlap + 0.6*0.5 from uninfected = 0.4
        //   merge iv2 into strain 1: coverage = 0.3 + 0.4 = 0.7, effect = 1.0
        // IV3: strain 2, cov=0.5, eff=1.0
        //   overlap with strain 1 (0.7): 0.5*0.7/2 = 0.175 each → strain 1 → 0.525
        //   strain 2 gets 0.175 from overlap + 0.3*0.5 from uninfected = 0.325
        // Final: strain 1 = 0.525, strain 2 = 0.325
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A", 1, 0.4f, 1.0f);
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A", 1, 0.5f, 1.0f);
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A", 2, 0.5f, 1.0f);

        auto result = m_pNEC->GetLarvalMicrosporidiaInfectivity(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A");

        CHECK_EQUAL(MAX_MICROSPORIDIA_STRAINS, (int)result.size());
        CHECK_CLOSE(0.0f,   result[0], FLT_EPSILON);
        CHECK_CLOSE(0.525f, result[1], 0.001f);
        CHECK_CLOSE(0.325f, result[2], 0.001f);
    }

    TEST_FIXTURE(InfectivityFixture, TestInfectivity_TwoStrainsDifferentEffects)
    {
        // IV1: strain 1, cov=0.5, eff=0.6 → temp[1] = {0.5, 0.6}
        // IV2: strain 2, cov=0.5, eff=0.4
        //   c2ep = 0.5*0.5 / (0.6+0.4) = 0.25
        //   strain 1 → (1-0.5)*0.5 + 0.6*0.25 = 0.40
        //   iv2 gets 0.4*0.25 = 0.10 from overlap + 0.5*0.5 = 0.25 from uninfected = 0.35
        // Final: strain 1 = 0.40*0.6 = 0.24, strain 2 = 0.35*0.4 = 0.14
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A", 1, 0.5f, 0.6f);
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A", 2, 0.5f, 0.4f);

        auto result = m_pNEC->GetLarvalMicrosporidiaInfectivity(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A");

        CHECK_EQUAL(MAX_MICROSPORIDIA_STRAINS, (int)result.size());
        CHECK_CLOSE(0.0f,  result[0], FLT_EPSILON);
        CHECK_CLOSE(0.24f, result[1], 0.001f);
        CHECK_CLOSE(0.14f, result[2], 0.001f);
    }

    TEST_FIXTURE(InfectivityFixture, TestInfectivity_HabitatFilter)
    {
        // Registered for CONSTANT; query for TEMPORARY_RAINFALL → all zeros
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::CONSTANT, "species_A", 1, 1.0f, 1.0f);

        auto result = m_pNEC->GetLarvalMicrosporidiaInfectivity(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A");

        CHECK_EQUAL(true, result.empty());
    }

    TEST_FIXTURE(InfectivityFixture, TestInfectivity_AllHabitatsMatchesAny)
    {
        // ALL_HABITATS registered; must match any specific habitat query
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::ALL_HABITATS, "species_A", 1, 0.6f, 1.0f);

        auto result_rain = m_pNEC->GetLarvalMicrosporidiaInfectivity(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A");
        auto result_const = m_pNEC->GetLarvalMicrosporidiaInfectivity(
            VectorHabitatType::CONSTANT, "species_A");

        CHECK_EQUAL(MAX_MICROSPORIDIA_STRAINS, (int)result_rain.size());
        CHECK_CLOSE(0.6f, result_rain[1], FLT_EPSILON);

        CHECK_EQUAL(MAX_MICROSPORIDIA_STRAINS, (int)result_const.size());
        CHECK_CLOSE(0.6f, result_const[1], FLT_EPSILON);
    }

    TEST_FIXTURE(InfectivityFixture, TestInfectivity_SpeciesFilter)
    {
        // Registered for species_B; query for species_A → all zeros
        m_pNEC->UpdateLarvalMicrosporidiaInterventions(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_B", 1, 1.0f, 1.0f);

        auto result = m_pNEC->GetLarvalMicrosporidiaInfectivity(
            VectorHabitatType::TEMPORARY_RAINFALL, "species_A");

        CHECK_EQUAL(true, result.empty());
    }
}
