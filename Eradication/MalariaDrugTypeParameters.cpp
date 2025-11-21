
#include "stdafx.h"
#include "MalariaDrugTypeParameters.h"
#include "IStrainIdentity.h"
#include "StrainIdentityMalariaGenetics.h"
#include "ParasiteGenetics.h"
#include "Debug.h"
#include "Common.h"
#include "Exceptions.h"
#include "Log.h"

SETUP_LOGGING( "MalariaDrugTypeParameters" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- DoseFractionByAge
    // ------------------------------------------------------------------------

    DoseFractionByAge::DoseFractionByAge()
        : JsonConfigurable()
        , m_AgeDays( 0.0 )
        , m_DoseFraction( 1.0 )
    {
    }

    DoseFractionByAge::~DoseFractionByAge()
    {
    }

    bool DoseFractionByAge::Configure( const Configuration * inputJson )
    {
        float years = 0.0;

        initConfigTypeMap( "Upper_Age_In_Years",     &years,          Upper_Age_In_Years_DESC_TEXT,     0.0f, MAX_HUMAN_AGE, 0.0f );
        initConfigTypeMap( "Fraction_Of_Adult_Dose", &m_DoseFraction, Fraction_Of_Adult_Dose_DESC_TEXT, 0.0f,           1.0, 1.0f );

        bool is_configured = JsonConfigurable::Configure( inputJson );
        if( is_configured && !JsonConfigurable::_dryrun )
        {
            m_AgeDays = years * DAYSPERYEAR;
        }
        return is_configured;
    }

    float DoseFractionByAge::GetAgeDays() const
    {
        return m_AgeDays;
    }

    float DoseFractionByAge::GetDoseFraction() const
    {
        return m_DoseFraction;
    }

    // ------------------------------------------------------------------------
    // --- DoseMap
    // ------------------------------------------------------------------------

    DoseMap::DoseMap()
        : JsonConfigurableCollection( "Fractional_Dose_By_Upper_Age" )
    {
    }

    DoseMap::~DoseMap()
    {
    }

    bool CheckAge( const DoseFractionByAge* pLeft, const DoseFractionByAge* pRight )
    {
        return (pLeft->GetAgeDays() < pRight->GetAgeDays());
    }

    bool FindAge( const float leftAgeDays, const DoseFractionByAge* pRight )
    {
        return (leftAgeDays < pRight->GetAgeDays());
    }

    void DoseMap::CheckConfiguration()
    {
        std::sort( m_Collection.begin(), m_Collection.end(), CheckAge );
    }

    float DoseMap::GetFractionalDose( float ageInDays ) const
    {
        auto it = std::upper_bound( m_Collection.begin(), m_Collection.end(), ageInDays, FindAge );

        float fractional_dose = 1.0;
        if( it != m_Collection.end() )
        {
            fractional_dose = (*it)->GetDoseFraction();
        }
        return fractional_dose;
    }

    DoseFractionByAge* DoseMap::CreateObject()
    {
        return new DoseFractionByAge();
    }

    // ------------------------------------------------------------------------
    // --- DrugModifier
    // ------------------------------------------------------------------------

    DrugModifier::DrugModifier()
        : JsonConfigurable()
        , m_DrugString()
        , m_C50(1.0f)
        , m_MaxKilling(1.0f)
        , m_DrugMarkers()
    {
    }

    DrugModifier::~DrugModifier()
    {
    }

    bool DrugModifier::Configure(const Configuration* inputJson)
    {
        initConfigTypeMap( "Drug_Resistant_String",  &m_DrugString, DM_Drug_Resistant_String_DESC_TEXT,  "" );
        initConfigTypeMap( "PKPD_C50_Modifier",      &m_C50,        DM_PKPD_C50_Modifier_DESC_TEXT,      0.0f, 1000.0f, 1.0f );
        initConfigTypeMap( "Max_IRBC_Kill_Modifier", &m_MaxKilling, DM_Max_IRBC_Kill_Modifier_DESC_TEXT, 0.0f, 1000.0f, 1.0f );

        bool is_configured = JsonConfigurable::Configure( inputJson );
        if( !JsonConfigurable::_dryrun && is_configured )
        {
            m_DrugMarkers = ParasiteGenetics::GetInstance()->GetAllelesForDrugResistantString( m_DrugString );
        }
        return is_configured;
    }

    const std::string& DrugModifier::GetDrugResistantString() const
    {
        return m_DrugString;
    }

    float DrugModifier::GetC50() const
    {
        return m_C50;
    }

    float DrugModifier::GetMaxKilling() const
    {
        return m_MaxKilling;
    }

    bool DrugModifier::IsResistant( const ParasiteGenome& rGenome ) const
    {
        return rGenome.HasAllOfTheAlleles( m_DrugMarkers );
    }

    // ------------------------------------------------------------------------
    // --- DrugResistanceModifierCollection
    // ------------------------------------------------------------------------

    DrugResistanceModifierCollection::DrugResistanceModifierCollection()
        : JsonConfigurableCollection( "Resistances" )
    {
    }

    DrugResistanceModifierCollection::~DrugResistanceModifierCollection()
    {
    }

    void DrugResistanceModifierCollection::CheckConfiguration()
    {
    }

    float DrugResistanceModifierCollection::GetC50( const IStrainIdentity& rStrain ) const
    {
        const StrainIdentityMalariaGenetics* p_si_genetics = dynamic_cast<const StrainIdentityMalariaGenetics*>(&rStrain);
        if( p_si_genetics == nullptr )
        {
            // If this is nullptr, then we aren't using ParasiteGenetics
            return 1.0;
        }

        float c50 = 1.0;
        for( DrugModifier* p_mods : m_Collection )
        {
            if( p_mods->IsResistant( p_si_genetics->GetGenome() ) )
            {
                c50 *= p_mods->GetC50();
            }
        }
        return c50;
    }

    float DrugResistanceModifierCollection::GetMaxKilling( const IStrainIdentity& rStrain ) const
    {
        const StrainIdentityMalariaGenetics* p_si_genetics = dynamic_cast<const StrainIdentityMalariaGenetics*>(&rStrain);
        if( p_si_genetics == nullptr )
        {
            // If this is nullptr, then we aren't using ParasiteGenetics
            return 1.0;
        }

        float max_killing = 1.0;
        for( DrugModifier* p_mods : m_Collection )
        {
            if( p_mods->IsResistant( p_si_genetics->GetGenome() ) )
            {
                max_killing *= p_mods->GetMaxKilling();
            }
        }
        return max_killing;
    }

    DrugModifier* DrugResistanceModifierCollection::CreateObject()
    {
        return new DrugModifier();
    }

    // ------------------------------------------------------------------------
    // --- MalariaDrugTypeParameters
    // ------------------------------------------------------------------------

    MalariaDrugTypeParameters::MalariaDrugTypeParameters()
        : JsonConfigurable()
        , drug_name()
        , PKPD_model( PKPDModel::FIXED_DURATION_CONSTANT_EFFECT )
        , max_drug_IRBC_kill(5.0f)
        , drug_hepatocyte_killrate(0.0f)
        , drug_gametocyte02_killrate( 0.0f )
        , drug_gametocyte34_killrate( 0.0f )
        , drug_gametocyteM_killrate( 0.0f )
        , drug_pkpd_c50(100.0f)
        , drug_Cmax(1000.0f)
        , drug_Vd(10.0f)
        , drug_decay_T1(1.0f)
        , drug_decay_T2(1.0f)
        , drug_fulltreatment_doses(3)
        , drug_dose_interval(1.0f)
        , bodyweight_exponent(0.0f)
        , dose_map()
        , m_Modifiers()
    {
    }

    MalariaDrugTypeParameters::~MalariaDrugTypeParameters()
    {
    }

    bool MalariaDrugTypeParameters::Configure(const ::Configuration* config)
    {
        LOG_DEBUG( "Configure\n" );

        initConfig( "PKPD_Model", PKPD_model, config, MetadataDescriptor::Enum( PKPD_Model_DESC_TEXT, PKPD_Model_DESC_TEXT, MDD_ENUM_ARGS( PKPDModel ) ) );

        initConfigTypeMap( "Name",                       &drug_name,                  Drug_Name_DESC_TEXT,                  std::string("") );
        initConfigTypeMap( "Max_Drug_IRBC_Kill",         &max_drug_IRBC_kill,         Max_Drug_IRBC_Kill_DESC_TEXT,         0.0f, 100000.0f, 5.0f );
        initConfigTypeMap( "Drug_Hepatocyte_Killrate",   &drug_hepatocyte_killrate,   Drug_Hepatocyte_Killrate_DESC_TEXT,   0.0f, 100000.0f, 0.0f );
        initConfigTypeMap( "Drug_Gametocyte02_Killrate", &drug_gametocyte02_killrate, Drug_Gametocyte02_Killrate_DESC_TEXT, 0.0f, 100000.0f, 0.0f );
        initConfigTypeMap( "Drug_Gametocyte34_Killrate", &drug_gametocyte34_killrate, Drug_Gametocyte34_Killrate_DESC_TEXT, 0.0f, 100000.0f, 0.0f );
        initConfigTypeMap( "Drug_GametocyteM_Killrate",  &drug_gametocyteM_killrate,  Drug_GametocyteM_Killrate_DESC_TEXT,  0.0f, 100000.0f, 0.0f );
        initConfigTypeMap( "Drug_PKPD_C50",              &drug_pkpd_c50,              Drug_PKPD_C50_DESC_TEXT,              0.0f, 100000.0f, 100.0f, "PKPD_Model", "CONCENTRATION_VERSUS_TIME" );
        initConfigTypeMap( "Drug_Cmax",                  &drug_Cmax,                  Drug_Cmax_DESC_TEXT,                  0.0f, 100000.0f, 1000.0f, "PKPD_Model", "CONCENTRATION_VERSUS_TIME" );
        initConfigTypeMap( "Drug_Vd",                    &drug_Vd,                    Drug_Vd_DESC_TEXT,                    0.0f, 100000.0f, 10.0f, "PKPD_Model", "CONCENTRATION_VERSUS_TIME" );
        initConfigTypeMap( "Drug_Decay_T1",              &drug_decay_T1,              Drug_Decay_T1_DESC_TEXT,              0.0f, 100000.0f, 1.0f );
        initConfigTypeMap( "Drug_Decay_T2",              &drug_decay_T2,              Drug_Decay_T2_DESC_TEXT,              0.0f, 100000.0f, 1.0f );
        initConfigTypeMap( "Drug_Fulltreatment_Doses",   &drug_fulltreatment_doses,   Drug_Fulltreatment_Doses_DESC_TEXT,   1,    100000,    3 );
        initConfigTypeMap( "Drug_Dose_Interval",         &drug_dose_interval,         Drug_Dose_Interval_DESC_TEXT,         0.0f, 100000.0f, 1.0f );
        initConfigTypeMap( "Bodyweight_Exponent",        &bodyweight_exponent,        DRUG_Bodyweight_Exponent_DESC_TEXT,   0.0f, 100000.0f, 0.0f );

        initConfigComplexCollectionType( "Resistances", &m_Modifiers, Resistance_DESC_TEXT, "Malaria_Model", "MALARIA_MECHANISTIC_MODEL_WITH_PARASITE_GENETICS" );

        if(JsonConfigurable::_dryrun || config->Exist("Fractional_Dose_By_Upper_Age")) // :(
        {
            initConfigComplexCollectionType( "Fractional_Dose_By_Upper_Age", &dose_map, Fractional_Dose_By_Upper_Age_DESC_TEXT );
        }

        bool is_configured = JsonConfigurable::Configure( config );

        if( is_configured && !JsonConfigurable::_dryrun )
        {
            if( drug_name.empty() )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                     "'Name' is set to empty string.\nDrugs must have 'Name' defined, not empty, and unique." );
            }

            // Check validity of dosing regimen
            float sim_tstep = (*EnvPtr->Config)["Simulation_Timestep"].As<Number>();
            float updates_per_tstep = (*EnvPtr->Config)["Infection_Updates_Per_Timestep"].As<Number>();
            if( drug_dose_interval < sim_tstep/updates_per_tstep )
            {
                std::ostringstream oss;
                oss << "Invalid 'Drug_Dose_Interval' in drug '" << drug_name << "'.\n";
                oss << "'Drug_Dose_Interval'(=" << drug_dose_interval << ") is less than the drug update period.\n";
                oss << "The drug update period = 'Simulation_Timestep'(=" << sim_tstep << ") / 'Infection_Updates_Per_Timestep'(=" << updates_per_tstep << ") = " << (sim_tstep / updates_per_tstep) << ".\n";
                oss << "Please change these parameters so that 'Drug_Dose_Interval' >= the drug update period.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, oss.str().c_str() );
            }
            
            if( config->Exist( "Resistances" ) )
            {
                m_Modifiers.CheckConfiguration();
            }
            if( config->Exist( "Fractional_Dose_By_Upper_Age" ) )
            {
                dose_map.CheckConfiguration();
            }
        }

        return is_configured;
    }

    const std::string& MalariaDrugTypeParameters::GetName() const
    {
        return drug_name;
    }

    PKPDModel::Enum MalariaDrugTypeParameters::GetPKPDModel() const
    {
        return PKPD_model;
    }

    float MalariaDrugTypeParameters::GetMaxDrugIRBCKill() const
    {
        return max_drug_IRBC_kill;
    }

    float MalariaDrugTypeParameters::GetKillRateHepatocyte() const
    {
        return drug_hepatocyte_killrate;
    }

    float MalariaDrugTypeParameters::GetKillRateGametocyte02() const
    {
        return drug_gametocyte02_killrate;
    }

    float MalariaDrugTypeParameters::GetKillRateGametocyte34() const
    {
        return drug_gametocyte34_killrate;
    }

    float MalariaDrugTypeParameters::GetKillRateGametocyteM() const
    {
        return drug_gametocyteM_killrate;
    }

    float MalariaDrugTypeParameters::GetPkpdC50() const
    {
        return drug_pkpd_c50;
    }

    float MalariaDrugTypeParameters::GetCMax() const
    {
        return drug_Cmax;
    }

    float MalariaDrugTypeParameters::GetVd() const
    {
        return drug_Vd;
    }

    float MalariaDrugTypeParameters::GetDecayT1() const
    {
        return drug_decay_T1;
    }

    float MalariaDrugTypeParameters::GetDecayT2() const
    {
        return drug_decay_T2;
    }

    int MalariaDrugTypeParameters::GetFullTreatmentDoses() const
    {
        return drug_fulltreatment_doses;
    }

    float MalariaDrugTypeParameters::GetDoseInterval() const
    {
        return drug_dose_interval;
    }

    float MalariaDrugTypeParameters::GetBodyWeightExponent() const
    {
        return bodyweight_exponent;
    }

    const DoseMap& MalariaDrugTypeParameters::GetDoseMap() const
    {
        return dose_map;
    }

    const DrugResistanceModifierCollection& MalariaDrugTypeParameters::GetResistantModifiers() const
    {
        return m_Modifiers;
    }

    // ------------------------------------------------------------------------
    // --- MalariaDrugTypeCollection
    // ------------------------------------------------------------------------

    MalariaDrugTypeCollection* MalariaDrugTypeCollection::m_pInstance = nullptr;

    MalariaDrugTypeCollection* MalariaDrugTypeCollection::GetInstanceNonConst()
    {
        if( m_pInstance == nullptr )
        {
            m_pInstance = new MalariaDrugTypeCollection();
        }
        return m_pInstance;
    }

    const MalariaDrugTypeCollection* MalariaDrugTypeCollection::GetInstance()
    {
        return GetInstanceNonConst();
    }

    void MalariaDrugTypeCollection::DeleteInstance()
    {
        delete m_pInstance;
        m_pInstance = nullptr;
    }

    MalariaDrugTypeCollection::MalariaDrugTypeCollection()
        : JsonConfigurableCollection( "Malaria_Drug_Params" )
        , m_DrugNames()
    {
    }

    MalariaDrugTypeCollection::~MalariaDrugTypeCollection()
    {
    }

    void MalariaDrugTypeCollection::CheckConfiguration()
    {
        for( auto p_drug : m_Collection )
        {
            m_DrugNames.insert( p_drug->GetName() );
        }

        if( m_DrugNames.size() != m_Collection.size() )
        {
            std::stringstream ss;
            ss << "Duplicate drug name.\n";
            ss << "The names of the species in 'Vector_Species_Params' must be unique.\n";
            ss << "The following names are defined:\n";
            for( auto p_drug : m_Collection )
            {
                ss << p_drug->GetName() << "\n";
            }
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    const jsonConfigurable::tDynamicStringSet& MalariaDrugTypeCollection::GetDrugNames() const
    {
        return m_DrugNames;
    }

    const MalariaDrugTypeParameters& MalariaDrugTypeCollection::GetDrug( const std::string& rName ) const
    {
        MalariaDrugTypeParameters* p_found = nullptr;
        for( auto p_drug : m_Collection )
        {
            if( p_drug->GetName() == rName )
            {
                p_found = p_drug;
            }
        }
        if( p_found == nullptr )
        {
            std::stringstream ss;
            ss << "'" << rName << "' is an unknown drug.\n";
            ss << "Valid drug names are:\n";
            for( auto p_drug : m_Collection )
            {
                ss << p_drug->GetName() << "\n";
            }
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        return *p_found;
    }

    MalariaDrugTypeParameters* MalariaDrugTypeCollection::CreateObject()
    {
        return new MalariaDrugTypeParameters();
    }
}
