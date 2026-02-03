
#pragma once

#include "Configure.h"
#include "ParasiteGenome.h"

namespace Kernel
{
    struct IDistribution;
    class RANDOMBASE;

    // This is used to determine what portion of the VarGenes (MSP & Major) are random for each infection
    // The minor epitopes are random all the time
    ENUM_DEFINE(VarGeneRandomnessType, 
        ENUM_VALUE_SPEC(FIXED_NEIGHBORHOOD , 0) // MSP and Major are fixed for the genome and only change due to recombination
        ENUM_VALUE_SPEC(FIXED_MSP          , 1) // MSP if fixed by the Major epitopes are random in every infection
        ENUM_VALUE_SPEC(ALL_RANDOM         , 2) // MSP and Major are random with every infection
    )

    struct LocationIndex
    {
        int32_t genome_location;
        int32_t nucleotide_index;

        LocationIndex( int32_t loc=0, int32_t nsIndex=0 )
            : genome_location( loc )
            , nucleotide_index( nsIndex )
        {
        }

        bool operator() ( const LocationIndex &lhs, const int32_t rhsLocation )
        {
            return lhs.genome_location < rhsLocation;
        }
    };

    class ParasiteGenetics : public JsonConfigurable
    {
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()
    public:
        friend class ParasiteGenomeInner;
        friend class ParasiteGenome;

        static int32_t NUM_CHROMOSOMES;
        static int32_t CHROMOSOME_LENGTH[];
        static std::vector<int32_t> CHROMOSOME_ENDS;
        static int32_t MAX_LOCATIONS;
        static int32_t FindChromosome( int32_t genomeLocation );
        // static flag set by ReportFpgNewInfections to tell ParasiteGenomes when it needs the crossover data
        // so ParasiteGenomes can store it for reporting 
        static bool collecting_parasite_genome_crossover_data;

        static ParasiteGenetics* CreateInstance();
        static void DeleteInstance();
        static const ParasiteGenetics* GetInstance();
        static void serialize( IArchive& ar );

        virtual bool Configure( const Configuration* pInputJson ) override;

        bool IsFPGSimulatingBaseModel() const;
        uint32_t ReduceSporozoitesDueToDeath( RANDOMBASE* pRNG, float dt, uint32_t numSporozoites, float mortalityModifer ) const;
        uint32_t ConvertOocystsToSporozoites( RANDOMBASE* pRNG, float dt, uint32_t numOocysts ) const;
        uint32_t GetNumSporozoitesInBite( RANDOMBASE* pRNG ) const;
        uint32_t GetNumOocystsFromBite( RANDOMBASE* pRNG ) const;

        int32_t GetSecondaryCrossoverDistance( RANDOMBASE* pRNG ) const;

        bool IsRandomMSP() const;
        bool IsRandomPfEMP1Major() const;

        int32_t GetNeighborhoodSizeMSP() const;
        int32_t GetNeighborhoodSizePfEMP1Major() const;

        const std::vector<int32_t>& GetIndexesBarcode() const;
        const std::vector<int32_t>& GetIndexesDrugResistant() const;
        const std::vector<int32_t>& GetIndexesHRP() const;
        const std::vector<int32_t>& GetIndexesPfEMP1Major() const;
        int32_t GetIndexMSP() const;

        std::string GetLocationParameterName( GenomeLocationType::Enum locType ) const;

        std::vector<std::pair<GenomeLocationType::Enum, int32_t>> GetLocations() const;
        const std::vector<int32_t>& GetLocationsBarcode() const;
        const std::vector<int32_t>& GetLocationsDrugResistant() const;
        const std::vector<int32_t>& GetLocationsHRP() const;
        const std::vector<int32_t>& GetLocationsPfEMP1Major() const;
        int32_t GetLocationMSP() const;

        int32_t ConvertLocationToIndex( int32_t genomeLocation ) const;
        int32_t ConvertCrossoverLocationToIndex( int32_t iChromosome, int32_t genomeLocation ) const;
        bool ChromosomeHasLocationsOfInterest( int32_t iChromosome ) const;
        int32_t GetFirstIndexOnChromosome( int32_t iChromosome ) const;
        int32_t GetLastIndexOnChromosome( int32_t iChromosome ) const;
        int32_t GetNumBasePairs() const;

        ParasiteGenomeAlleleCollection GetAllelesForDrugResistantString( const std::string& rDrugString ) const;
        ParasiteGenomeAlleleCollection GetAllelesForHrpString( const std::string& rHrpString ) const;

        ParasiteGenome CreateGenome( const ParasiteGenome& rGenome, uint32_t infectionID ) const;
        ParasiteGenome CreateGenome( const std::string& rBarcodeString, const std::vector<int32_t>& rRoots ) const;
        ParasiteGenomeInner* TEST_CreateGenomeInner( const std::string& rBarcodeString, const std::vector<int32_t>& rRoots ) const;
        ParasiteGenome CreateGenomeFromBarcode( RANDOMBASE* pRNG,
                                                const std::string& rBarcodeString,
                                                const std::string& rDrugString,
                                                const std::string& rHrpString ) const;
        ParasiteGenome CreateGenomeFromSequence( RANDOMBASE* pRNG,
                                                 const std::string& rBarcodeString,
                                                 const std::string& rDrugString,
                                                 const std::string& rHrpString,
                                                 int32_t mspValues,
                                                 const std::vector<int32_t>& rPfEMP1MajorValues ) const;
        ParasiteGenome CreateGenomeFromAlleleFrequencies( RANDOMBASE* pRNG,
                                                          const std::vector<std::vector<float>>& rAlleleFreqBarcode,
                                                          const std::vector<std::vector<float>>& rAlleleFreqDrugResistant,
                                                          const std::vector<std::vector<float>>& rAlleleFreqHRP ) const;

        std::vector<int64_t> FindPossibleBarcodeHashcodes( const std::string& rParameterName,
                                                           const std::string& rBarcode ) const;

        void ReduceGenomeMap();
        void ClearGenomeMap();
        bool CheckHashcodes( ParasiteGenomeInner* pInner ) const;
        uint32_t GetGenomeMapSize() const;

        static int32_t ConvertCharToVal( const std::string& rParameterName, bool isReportParameter, char c );

    protected:
        uint32_t GetNextGenomeID();
        ParasiteGenomeInner* AddParasiteGenomeInner( ParasiteGenomeInner* pInner );
        void CheckLocationMSP( int indexOther,
                               int32_t locationOther,
                               const char* pOtherVariableName );
        void CheckLocationMajor( int indexOther,
                                 int32_t locationOther,
                                 const char* pOtherVariableName );
        void CheckForDuplicateLocations();
        void OrganizeNucleotideSquenceParameters();
        float AddBarcodeValues( const std::string& rBarcodeString, std::vector<int32_t>& rSequence ) const;
        void AddDrugResistantValues( const std::string& rDrugString, std::vector<int32_t>& rSequence ) const;
        void AddHrpValues( const std::string& rHrpString, std::vector<int32_t>& rSequence ) const;
        void AddVarGenes( RANDOMBASE* pRNG, float barcodeDistance, std::vector<int32_t>& rSequence ) const;

        void CheckStringLength( const std::string& rParameterName,
                                bool isReportParameter,
                                const std::string& rString,
                                const char* pLocationsParameterName,
                                int32_t numLocations ) const;

        ParasiteGenomeAlleleCollection
        ParasiteGenetics::GetAllelesForString( GenomeLocationType::Enum locationType,
                                               const std::string& rString,
                                               const std::vector<int32_t>& rLocations,
                                               const std::vector<int32_t>& rIndeses,
                                               const char* pParamNameString,
                                               const char* pParamNameLocations,
                                               const char* pZeroLocationsMessage ) const;


    private:
        static ParasiteGenetics* m_pInstance;

        ParasiteGenetics();
        ParasiteGenetics( const ParasiteGenetics& rMaster ) = delete;
        virtual ~ParasiteGenetics();

        IDistribution* m_pDistributionSporozoitesPerOocyst;

        bool  m_IsFPGSimulatingBaseModel;
        float m_SporozoiteMortalityRate;
        float m_NumSporozoitesInBiteFails;
        float m_ProbabilitySporozoiteInBiteFails;
        float m_NumOocystFromBiteFail;
        float m_ProbabilityOocystFromBiteFails;
        float m_CrossoverGammaK;
        float m_CrossoverGammaTheta;

        VarGeneRandomnessType::Enum m_VarGeneRandomnessType;
        int32_t m_NeighborhoodSizeMSP;
        int32_t m_NeighborhoodSizeMajor;

        std::vector<std::pair<GenomeLocationType::Enum, int32_t>> m_GenomeLocationAndTypes;

        std::vector<int32_t> m_LocationsBarcode;
        std::vector<int32_t> m_LocationsDrug;
        std::vector<int32_t> m_LocationsHRP;
        std::vector<int32_t> m_LocationsMajor;
        int32_t              m_LocationMSP;

        std::vector<int32_t> m_IndexesBarcode;
        std::vector<int32_t> m_IndexesDrug;
        std::vector<int32_t> m_IndexesHRP;
        std::vector<int32_t> m_IndexesMajor;
        int32_t              m_IndexMSP;

        std::vector<std::vector<LocationIndex>> m_LocationIndexesPerChromosome;

        int32_t m_NumBasePairs;

        std::map<int64_t,ParasiteGenomeInner*> m_ParasiteGenomeMap;
        std::map<int64_t, uint32_t> m_HashToGenomeID;
        suids::distributed_generator m_GenomeIdGenerator;
    };
}
