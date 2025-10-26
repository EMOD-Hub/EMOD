
#pragma once

#include "IIndividualHumanContext.h"
#include "Interventions.h"
#include "HIVInterventionsContainer.h"
#include "MalariaInterventionsContainerContexts.h"

using namespace Kernel;

class IndividualHumanInterventionsContextFake : public IIndividualHumanInterventionsContext,
                                                public IInterventionConsumer,
                                                public IHIVMedicalHistory,
                                                public IMalariaDrugEffectsApply,
                                                public IMalariaDrugEffects,
                                                public IHIVDrugEffectsApply
{
public:
    IndividualHumanInterventionsContextFake()
        : IIndividualHumanInterventionsContext()
        , m_Parent(nullptr)
        , m_CD4Count(0.0)
        , m_CD4CountLowest(FLT_MAX)
        , m_ReceivedCD4(false)
        , m_EverStaged(false)
        , m_OnStageForArt(false)
        , m_HasTested(false)
        , m_HasTestedHIVPositive(false)
        , m_ReceivedTestResultForHIV(ReceivedTestResultsType::UNKNOWN)
        , m_WhoStage(0.0)
        , m_MalariaDrugEffects()
        , m_InterventionsContainer( nullptr )
        , m_InterventionsList()
    {
    }
    virtual ~IndividualHumanInterventionsContextFake() {}

    // -------------------------------------------------
    // --- IIndividualHumanInterventionsContext Methods
    // -------------------------------------------------
    virtual void SetContextTo(IIndividualHumanContext *context)
    {
        m_Parent = context ;
    }

    virtual IIndividualHumanContext* GetParent()
    {
        return m_Parent ;
    }

    virtual std::list<void*> GetInterventionsByInterface( iid_t iid )
    { 
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); 
    }

    virtual void PurgeExistingByType( const std::string &type_name )
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual void PurgeExistingByName( const std::string& type_name, const InterventionName& intervention_name )
    {
        throw Kernel::NotYetImplementedException(__FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual const std::vector<IDistributableIntervention*>& GetInterventions() const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
    }

    virtual void ChangeProperty( const char *property, const char* new_value ) override
    {
        if( m_Parent != nullptr )
        {
            IPKeyValue kv( property, new_value );
            m_Parent->GetEventContext()->GetProperties()->Set( kv );
        }
    }

    uint32_t GetNumInterventions() const override
    {
        return 0;
    }

    uint32_t GetNumInterventionsAdded() override
    {
        return 0;
    }

    virtual bool ContainsExistingByName( const InterventionName& name ) override
    {
        for( auto p_iv : m_InterventionsList )
        {
            if( p_iv->GetName() == name )
            {
                return true;
            }
        }
        return false;
    }

    virtual IPKeyValue& GetLastIPChange() const override
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
    }
    virtual void CheckSTINetworkParams(const char *prop = nullptr, const char* new_value = nullptr) {}

    // ---------------------
    // --- ISupport Methods
    // ---------------------
    virtual QueryResult QueryInterface(iid_t iid, void **ppvObject)
    {
        *ppvObject = nullptr ;
        if (iid == GET_IID(IInterventionConsumer))
            *ppvObject = static_cast<IInterventionConsumer*>(this);
        else if (iid == GET_IID(IHIVMedicalHistory))
            *ppvObject = static_cast<IHIVMedicalHistory*>(this);
        else if (iid == GET_IID(IMalariaDrugEffectsApply))
            *ppvObject = static_cast<IMalariaDrugEffectsApply*>(this);
        else if (iid == GET_IID(IMalariaDrugEffects))
            *ppvObject = static_cast<IMalariaDrugEffects*>(this);
        else if (iid == GET_IID(IHIVDrugEffectsApply))
            *ppvObject = static_cast<IHIVDrugEffectsApply*>(m_InterventionsContainer);

        if( *ppvObject != nullptr )
        {
            return QueryResult::s_OK ;
        }
        else
            return QueryResult::e_NOINTERFACE ;
    }

    virtual int32_t AddRef()
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }
    virtual int32_t Release()
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    // ------------------------------
    // --- IInterventionConsumer Methods
    // ------------------------------
    virtual bool GiveIntervention( IDistributableIntervention * pIV )
    {
        m_InterventionsList.push_back( pIV );
        return true ;
    }

    // ------------------------------
    // --- IHIVMedicalHistory Methods
    // ------------------------------

    virtual void OnTestCD4(float cd4count)
    {
        m_CD4Count = cd4count ;
        if( m_CD4Count < m_CD4CountLowest )
        {
            m_CD4CountLowest = m_CD4Count ;
        }
        m_ReceivedCD4 = true ;
    }

    virtual float FirstRecordedCD4() const 
    {
        return -1.0 ;
    }

    virtual float LastRecordedCD4() const 
    {
        return m_CD4Count ;
    }

    virtual float LowestRecordedCD4() const
    {
        return m_CD4CountLowest ;
    }

    virtual bool EverReceivedCD4() const
    {
        return m_ReceivedCD4 ;
    }

    virtual void OnStageForART(bool stagedForART)
    {
        m_EverStaged = true ;
        m_OnStageForArt = stagedForART ;
    }

    virtual bool EverStaged() const
    {
        return m_EverStaged ;
    }

    virtual bool EverStagedForART() const
    {
        return m_OnStageForArt ;
    }

    virtual void OnAssessWHOStage(float WHOStage)
    {
        m_WhoStage = WHOStage ;
    }

    virtual float LastRecordedWHOStage() const
    {
        return m_WhoStage ;
    }

    virtual bool EverTested() const
    {
        return m_HasTested ;
    }

    virtual bool EverTestedPastYear() const { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    virtual bool EverTestedHIVPositive() const
    {
        return m_HasTestedHIVPositive ;
    }

    virtual ReceivedTestResultsType::Enum ReceivedTestResultForHIV() const
    {
        return m_ReceivedTestResultForHIV ;
    }

    virtual void OnTestForHIV(bool test_result)
    {
        m_HasTested = true ;
        if( test_result )
        {
            m_HasTestedHIVPositive = true ;
        }
    }

    virtual void OnReceivedTestResultForHIV(bool test_result)
    {
        if( test_result )
            m_ReceivedTestResultForHIV = ReceivedTestResultsType::POSITIVE ;
        else
            m_ReceivedTestResultForHIV = ReceivedTestResultsType::NEGATIVE ;
    }

    virtual void OnBeginART()                     { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual bool EverBeenOnART()         const { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    virtual float TimeOfMostRecentTest()      const { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float TimeOfMostRecentCD4()       const { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float TimeLastSeenByHealthcare()  const { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float TimeFirstStartedART()       const { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float TimeLastStartedART()        const { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float TotalTimeOnART()            const { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual unsigned int NumTimesStartedART() const { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    virtual NaturalNumber    GetTotalARTInitiations()     const { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual NonNegativeFloat GetTotalYearsOnART()         const { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual NonNegativeFloat GetYearsSinceFirstARTInit()  const { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual NonNegativeFloat GetYearsSinceLatestARTInit() const { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    // IMalariaDrugEffects
    virtual float get_drug_IRBC_killrate( const IStrainIdentity& rStrain ) { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float get_drug_hepatocyte(    const IStrainIdentity& rStrain ) { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float get_drug_gametocyte02(  const IStrainIdentity& rStrain ) { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float get_drug_gametocyte34(  const IStrainIdentity& rStrain ) { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float get_drug_gametocyteM(   const IStrainIdentity& rStrain ) { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    //IMalariaDrugEffectsApply
    virtual void AddDrugEffects(    IMalariaDrugEffects* pDrugEffects )
    {
        m_MalariaDrugEffects.push_back( pDrugEffects );
    }

    virtual void RemoveDrugEffects( IMalariaDrugEffects* pDrugEffects )
    {
        auto it = std::find( m_MalariaDrugEffects.begin(), m_MalariaDrugEffects.end(), pDrugEffects );
        if( it == m_MalariaDrugEffects.end() )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Did not find Malaria Drug Effect" );
        }
        m_MalariaDrugEffects.erase( it );
    }

    //IHIVDrugEffectsApply
    virtual void ApplyDrugConcentrationAction(std::string, float current_concentration) {}
    virtual void ApplyDrugVaccineReducedAcquireEffect(float rate) {}
    virtual void ApplyDrugVaccineReducedTransmitEffect(float rate) {}
    virtual void ApplyDrugInactivationRateEffect(float rate) {}
    virtual void ApplyDrugClearanceRateEffect(float rate) {}
    virtual void ApplyProbMaternalTransmissionModifier(const ProbabilityNumber &probReduction) {}
    virtual void GoOnART(bool viralSupression,
        float daysToAchieveSuppression,
        float durationFromEnrollmentToArtAidsDeath,
        float artMultiplierOnTransmissionProbPerAct) {}
    virtual void GoOffART() {}

    // --------------------
    // --- Other Methods
    // --------------------
    bool HasDrugEffects() const
    {
        return (m_MalariaDrugEffects.size() > 0);
    }

    void SetInterventionsContainer(HIVInterventionsContainer* container)
    {
        m_InterventionsContainer = container;
    }

private:
    IIndividualHumanContext* m_Parent ;
    float m_CD4Count ;
    float m_CD4CountLowest ;
    bool m_ReceivedCD4 ;
    bool m_EverStaged ;
    bool m_OnStageForArt ;
    bool m_HasTested ;
    bool m_HasTestedHIVPositive ;
    ReceivedTestResultsType::Enum m_ReceivedTestResultForHIV ;
    float m_WhoStage ;
    std::vector<IMalariaDrugEffects*> m_MalariaDrugEffects;
    HIVInterventionsContainer* m_InterventionsContainer;
    std::vector<IDistributableIntervention*> m_InterventionsList;
};
